/********************************************************************
 * VMU Backup CD Special Edition (Feb/2005)
 * coded by El Bucanero
 *
 * some code based on <kos/examples/dreamcast/png/> from KallistiOS
 *
 * Copyright (C) 2005 Damian Parrino <bucanero@fibertel.com.ar>
 * http://www.bucanero.com.ar/
 *
 * Greetz to:
 * Dan Potter for KallistiOS and other great libs like Tsunami
 * AndrewK / Napalm 2001 for the lovely DC load-ip/tool				<andrewk@napalm-x.com>
 * Lawrence Sebald for the MinGW/msys cross compiler tutorial
 * Dreamcast@PlanetWeb for the whole bunch of DC saves				<http://dreamcast.planetweb.com/>
 * Jeff.Ma for sharing a lot of JAP & other rare saves				<http://www.dreamcastcn.com/>
 *
 * and last, but not least, thanks to SEGA for my ALWAYS LOVED Dreamcast! =)
 *
 ********************************************************************/

#include <kos.h>
#include <png/png.h>
#include <zlib/zlib.h>
#include "vmu_icon.h"
#include "buk_icon.h"
#include "s3mplay.h"

#define draw_pixel(x,y,color) vram_s[(y)*640+(x)]=(color)
#define PACK_RGB565(r,g,b) (((r>>3)<<11)|((g>>2)<<5)|((b>>3)<<0))

// GUI constants
#define VMU_X 510
#define VMU_Y 210
#define VMU_RGB 0, 0, 0
#define VMU_MAX_PAGE 3
#define GAMES_X 30
#define GAMES_Y 35
#define GAMES_LINE_LENGTH 54
#define GAMES_RGB 0, 0, 0
#define GAMES_MAX_PAGE 15
#define SAVES_X 515
#define SAVES_Y 35
#define SAVES_RGB 0, 0, 0
#define SAVES_MAX_PAGE 7
#define INFO_X 30
#define INFO_Y 360
#define INFO_LINE_LENGTH 72
#define INFO_RGB 0, 0, 0
#define SELECTED_RGB 1, 1, 0
#define CREDITS_RGB 1, 1, 1

typedef struct game_list {
	char dir[32];
	char name[64];
	struct game_list *next;
} game_node_t;

typedef struct save_list {
	char name[13];
	char file[13];
	char desc[256];
	ssize_t size;
	struct save_list *next;
} save_node_t;

typedef struct vmu_list {
	char name[3];
	uint8 free_blocks;
	struct vmu_list *next;
} vmu_node_t;

typedef struct menu_struct {
	int top;
	int pos;
	int total;
	int maxpage;
} menu_pos_t;

typedef struct interface_struct {
	game_node_t *games;
	menu_pos_t *game_pos;
	save_node_t *saves;
	menu_pos_t *save_pos;
	vmu_node_t *vmus;
	menu_pos_t *vmu_pos;
} interface_t;

int file_select(interface_t inter, menu_pos_t *menu);
void update_lists(interface_t inter);
int load_game_list(game_node_t *gptr);
int load_save_list(save_node_t *sptr, char *dir);
int load_vmu_list(vmu_node_t *vptr);
void free_game_list(game_node_t *gptr);
void free_save_list(save_node_t *sptr);
void free_vmu_list(vmu_node_t *vptr);
char* get_game_directory(game_node_t *gptr, int pos);
save_node_t* get_save_ptr(save_node_t *sptr, int pos);
vmu_node_t* get_vmu_ptr(vmu_node_t *vptr, int pos);
void copy_file(interface_t inter);
int get_free_blocks(maple_device_t *dev);
void draw_frame(void);
void splash_screen(char *gzfile, int width, int height);
void credits_scroll();

extern uint8 romdisk[];
extern char wfont[];
volatile unsigned long *snd_dbg=(unsigned long*)0xa080ffc0;

pvr_ptr_t font_tex;
pvr_ptr_t back_tex;
char games_lst[1024];
char saves_lst[256];
char vmus_lst[64];
char vmu_info[384];

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

int file_select(interface_t inter, menu_pos_t *menu) {
	int done=0;
	uint64	timer = timer_ms_gettime64();

	while (!done) {
		update_lists(inter);
		draw_frame();
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, t)
			if ((t->buttons & CONT_DPAD_UP) && (menu->pos >= menu->top) && (timer + 200 < timer_ms_gettime64())) {
				if (menu->pos > menu->top) {
					menu->pos--;
				} else {
					if ((menu->pos == menu->top) && (menu->top != 0)) {
						menu->top=menu->top - menu->maxpage;
						menu->pos--;
					}
				}
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_DOWN) && (menu->pos <= menu->top + menu->maxpage) && (timer + 200 < timer_ms_gettime64())) {
				if ((menu->pos < menu->top + menu->maxpage - 1) && (menu->pos < menu->total)) {
					menu->pos++;
				} else {
					if ((menu->pos == menu->top + menu->maxpage - 1) && (menu->top + menu->maxpage - 1 < menu->total)) {
						menu->top=menu->top + menu->maxpage;
						menu->pos++;
					}
				}
				timer = timer_ms_gettime64();
			}
			if ((t->ltrig > 0) && (menu->top != 0) && (timer + 200 < timer_ms_gettime64())) {
				menu->top=menu->top - menu->maxpage;
				menu->pos=menu->top;
				timer = timer_ms_gettime64();
			}
			if ((t->rtrig > 0) && (menu->top + menu->maxpage - 1 < menu->total) && (timer + 200 < timer_ms_gettime64())) {
				menu->top=menu->top + menu->maxpage;
				menu->pos=menu->top;
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_A) && (timer + 200 < timer_ms_gettime64())) {
				if (inter.saves == NULL) {
					inter.save_pos->top=0;
					inter.save_pos->pos=0;
					inter.saves=(save_node_t *)malloc(sizeof(save_node_t));
					inter.save_pos->total=load_save_list(inter.saves, get_game_directory(inter.games, inter.game_pos->pos));
					file_select(inter, inter.save_pos);
					free_save_list(inter.saves);
					inter.saves=NULL;
				} else {
					if (inter.vmus == NULL) {
						inter.vmu_pos->top=0;
						inter.vmu_pos->pos=0;
						inter.vmus=(vmu_node_t *)malloc(sizeof(vmu_node_t));
						inter.vmu_pos->total=load_vmu_list(inter.vmus);
						file_select(inter, inter.vmu_pos);
						free_vmu_list(inter.vmus);
						inter.vmus=NULL;
					} else {
						copy_file(inter);
						done=1;
					}
				}
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_B) && (timer + 200 < timer_ms_gettime64())) {
				done=1;
//				menu->pos=-1;
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_Y) && (timer + 200 < timer_ms_gettime64())) {
				credits_scroll();
				timer = timer_ms_gettime64();
			}
		MAPLE_FOREACH_END()
	}
	return(menu->pos);
}

void update_lists(interface_t inter) {
	int		i=0;
	game_node_t	*gaux=inter.games;
	save_node_t	*saux=inter.saves;
	vmu_node_t *vaux=inter.vmus;

	strcpy(games_lst, "");
	while ((gaux->next != NULL) && (i < inter.game_pos->top + inter.game_pos->maxpage)) {
		if (i >= inter.game_pos->top) {
			if (i == inter.game_pos->pos) strcat(games_lst, "¬");
			sprintf(vmu_info, "%*s\n", (GAMES_LINE_LENGTH + strlen(gaux->name))/2, gaux->name);
			strcat(games_lst, vmu_info);
		}
		i++;
		gaux=gaux->next;
	}
	strcpy(vmus_lst, "");
	if (vaux != NULL) {
		i=0;
		while ((vaux->next != NULL) && (i < inter.vmu_pos->top + inter.vmu_pos->maxpage)) {
			if (i >= inter.vmu_pos->top) {
				if (i == inter.vmu_pos->pos) strcat(vmus_lst, "¬");
				sprintf(vmu_info, "VMU %c%c (%d)\n", vaux->name[0]-32, vaux->name[1], vaux->free_blocks);
				strcat(vmus_lst, vmu_info);
			}
			i++;
			vaux=vaux->next;
		}
	}
	strcpy(saves_lst, "");
	strcpy(vmu_info, "");
	if (saux != NULL) {
		i=0;
		while ((saux->next != NULL) && (i < inter.save_pos->top + inter.save_pos->maxpage)) {
			if (i >= inter.save_pos->top) {
				if (i == inter.save_pos->pos) {
					strcat(saves_lst, "¬");
					sprintf(vmu_info, "File: %s - %d Block(s)\n%s", saux->name, (saux->size / 512), saux->desc);
				}
				strcat(saves_lst, saux->file);
				strcat(saves_lst, "\n");
			}
			i++;
			saux=saux->next;
		}
	}
}

char* get_game_directory(game_node_t *gptr, int pos) {
	game_node_t *aux=gptr;
	int i=0;

	while (i < pos) {
		aux=aux->next;
		i++;
	}
	return(aux->dir);
}

save_node_t* get_save_ptr(save_node_t *sptr, int pos) {
	save_node_t *aux=sptr;
	int i=0;

	while (i < pos) {
		aux=aux->next;
		i++;
	}
	return(aux);
}

vmu_node_t* get_vmu_ptr(vmu_node_t *vptr, int pos) {
	vmu_node_t *aux=vptr;
	int i=0;

	while (i < pos) {
		aux=aux->next;
		i++;
	}
	return(aux);
}

void copy_file(interface_t inter) {
	save_node_t *sptr;
	vmu_node_t *vptr;
	char tmpsrc[64], tmpdst[64];
	uint64	timer;

	vmu_set_icon(buk_icon_xpm);
	sptr=get_save_ptr(inter.saves, inter.save_pos->pos);
	vptr=get_vmu_ptr(inter.vmus, inter.vmu_pos->pos);
	if ((sptr->size / 512) <= vptr->free_blocks) {
		sprintf(tmpsrc, "/cd/%s/%s", get_game_directory(inter.games, inter.game_pos->pos), sptr->file);
		sprintf(tmpdst, "/vmu/%s/%s", vptr->name, sptr->name);
// DEBUG
//		printf("%s\n%s\n", tmpsrc, tmpdst);
// DEBUG
		if (fs_copy(tmpsrc, tmpdst) == sptr->size) {
			sprintf(vmu_info, "%s: %d blocks copied to VMU %c%c.\n", sptr->name, sptr->size/512, vptr->name[0]-32, vptr->name[1]);
		} else {
			sprintf(vmu_info, "Copy Error!\n");
		}
	} else {
		sprintf(vmu_info, "Error! Not enough free blocks.\n");
	}
	timer = timer_ms_gettime64();
	while (timer + 1500 > timer_ms_gettime64()) {
		draw_frame();
	}
	vmu_set_icon(vmu_icon_xpm);
}

void back_init(char *txrfile) {
    back_tex = pvr_mem_malloc(512*512*2);
    png_to_texture(txrfile, back_tex, PNG_NO_ALPHA);
}

void font_init() {
    int i,x,y,c;
    unsigned short * temp_tex;
    
    font_tex = pvr_mem_malloc(256*256*2);
    temp_tex = (unsigned short *)malloc(256*128*2);
    
    c = 0;
    for(y = 0; y < 128 ; y+=16)
        for(x = 0; x < 256 ; x+=8) {
            for(i = 0; i < 16; i++) {
                temp_tex[x + (y+i) * 256 + 0] = 0xffff * ((wfont[c+i] & 0x80)>>7);
                temp_tex[x + (y+i) * 256 + 1] = 0xffff * ((wfont[c+i] & 0x40)>>6);
                temp_tex[x + (y+i) * 256 + 2] = 0xffff * ((wfont[c+i] & 0x20)>>5);
                temp_tex[x + (y+i) * 256 + 3] = 0xffff * ((wfont[c+i] & 0x10)>>4);
                temp_tex[x + (y+i) * 256 + 4] = 0xffff * ((wfont[c+i] & 0x08)>>3);
                temp_tex[x + (y+i) * 256 + 5] = 0xffff * ((wfont[c+i] & 0x04)>>2);
                temp_tex[x + (y+i) * 256 + 6] = 0xffff * ((wfont[c+i] & 0x02)>>1);
                temp_tex[x + (y+i) * 256 + 7] = 0xffff * (wfont[c+i] & 0x01);
            }
            c+=16;
        }
    pvr_txr_load_ex(temp_tex, font_tex, 256, 256, PVR_TXRLOAD_16BPP);
	free(temp_tex);
}

void draw_back(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;

    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, 512, 512, back_tex, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);    
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;
    
    vert.x = 1;
    vert.y = 1;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = 640;
    vert.y = 1;
    vert.z = 1;
    vert.u = 1.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = 1;
    vert.y = 480;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 1.0;
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = 640;
    vert.y = 480;
    vert.z = 1;
    vert.u = 1.0;
    vert.v = 1.0;
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));
}

void draw_char(float x1, float y1, float z1, float a, float r, float g, float b, int c, float xs, float ys) {
    pvr_vertex_t    vert;
    int             ix, iy;
    float           u1, v1, u2, v2;
    
    ix = (c % 32) * 8;
    iy = (c / 32) * 16;
    u1 = (ix + 0.5f) * 1.0f / 256.0f;
    v1 = (iy + 0.5f) * 1.0f / 256.0f;
    u2 = (ix+7.5f) * 1.0f / 256.0f;
    v2 = (iy+15.5f) * 1.0f / 256.0f;
    
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x1;
    vert.y = y1 + 16.0f * ys;
    vert.z = z1;
    vert.u = u1;
    vert.v = v2;
    vert.argb = PVR_PACK_COLOR(a,r,g,b);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = x1;
    vert.y = y1;
    vert.u = u1;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = x1 + 8.0f * xs;
    vert.y = y1 + 16.0f * ys;
    vert.u = u2;
    vert.v = v2;
    pvr_prim(&vert, sizeof(vert));
    
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x1 + 8.0f * xs;
    vert.y = y1;
    vert.u = u2;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));
}

void draw_string(float x, float y, float z, float a, float r, float g, float b, char *str, float xs, float ys, int max_len=0) {
	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	float orig_x = x;
	int i=0;

	pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 256, 256, font_tex, PVR_FILTER_BILINEAR);
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));  
	while (*str) {
	if (*str == '\r') {
		str++;
		continue;
	}
	if (*str == '¬') {
		str++;
		while (*str != '\n') {
			draw_char(x, y, z, a, SELECTED_RGB, *str++, xs, ys);
			x+=8*xs;
		}
	}
	if (*str == '\n') {
		x=orig_x;
		y+=16.0f*ys + 4.0f;
		str++;
		i=0;
		continue;
	}
	if ((max_len > 0) && (i == max_len)) {
		x=orig_x;
		y+=16.0f*ys + 4.0f;
		i=0;
	}
	draw_char(x, y, z, a, r, g, b, *str++, xs, ys);
	x+=8*xs;
	i++;
  }
}

void draw_frame(void) {
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);
    draw_back();
    pvr_list_finish();
    pvr_list_begin(PVR_LIST_TR_POLY);
	draw_string(GAMES_X, GAMES_Y, 3, 1, GAMES_RGB, games_lst, 1, 1);
    draw_string(SAVES_X, SAVES_Y, 3, 1, SAVES_RGB, saves_lst, 1, 1);
    draw_string(VMU_X, VMU_Y, 3, 1, VMU_RGB, vmus_lst, 1, 1);
    draw_string(INFO_X, INFO_Y, 3, 1, INFO_RGB, vmu_info, 1, 1, INFO_LINE_LENGTH);
    pvr_list_finish();
    pvr_scene_finish();
}

int load_game_list(game_node_t *gptr) {
	char *tok;
	void *buf;
	game_node_t *aux=gptr;
	int i=-1;

	fs_load("/cd/GAMELIST.TXT", &buf);
	tok=strtok((char *)buf, "=");
	while (tok != NULL) {
		strcpy(aux->dir, tok);
//		_strupr(aux->dir);
		tok=strtok(NULL, "\n");
		if (tok != NULL) {
			strcpy(aux->name, tok);
// DEBUG
//			printf("---%d---\n%s\n%s\n", i+1, aux->dir, aux->name);
// DEBUG
			tok=strtok(NULL, "=");
			aux->next=(game_node_t *)malloc(sizeof(game_node_t));
			aux=aux->next;
			i++;
		}
	}
	aux->next=NULL;
	free(buf);
	return(i);
}

int load_save_list(save_node_t *sptr, char *dir) {
	char tmp[64];
	char *tok;
	void *buf;
	save_node_t *aux=sptr;
	int i=-1;
	file_t	f;

	sprintf(tmp, "/cd/%s/SAVELIST.TXT", dir);
	fs_load(tmp, &buf);
	tok=strtok((char *)buf, "=");
	while (tok != NULL) {
		sprintf(tmp, "/cd/%s/%s", dir, tok);
//		sprintf(tmp, "/cd/%s/%s", dir, _strupr(tok));
		sprintf(vmu_info, "Loading... %s", tok);
		draw_frame();
		tok=strtok(NULL, "\n");
		if (tok != NULL) {
			strcpy(aux->desc, tok);
			f=fs_open(tmp, O_RDONLY);
			fs_seek(f, 0x50, SEEK_SET);
			fs_read(f, tmp, 8);
			sprintf(aux->file, "%.8s.VMS", tmp);
//			_strupr(aux->file);
			fs_read(f, aux->name, 13);
			fs_seek(f, 0x68, SEEK_SET);
			fs_read(f, &(aux->size), 4);
			fs_close(f);
// DEBUG
//			printf("---%d---\n%s\n%s\n%s\n%d\n", i+1, aux->name, aux->file, aux->desc, aux->size);
// DEBUG
			tok=strtok(NULL, "=");
			aux->next=(save_node_t *)malloc(sizeof(save_node_t));
			aux=aux->next;
			i++;
		}
	}
	aux->next=NULL;
	free(buf);
	return(i);
}

int load_vmu_list(vmu_node_t *vptr) {
	int i(0);
	vmu_node_t *aux=vptr;
	maple_device_t *dev=NULL;

	while ((dev=maple_enum_type(i++,MAPLE_FUNC_MEMCARD))!=NULL) {
		sprintf(aux->name, "%c%d", (97+dev->port), dev->unit);		
		aux->free_blocks=get_free_blocks(dev);
// DEBUG
//		printf("---%d---\n%s\n%d\n", i-1, aux->name, aux->free_blocks);
// DEBUG
		aux->next=(vmu_node_t *)malloc(sizeof(vmu_node_t));
		aux=aux->next;
	}
	aux->next=NULL;
	return(i-2);
}

int get_free_blocks(maple_device_t *dev) {
	uint16 buf16[256];
	int free_blocks(0),i(0);

	if(vmu_block_read(dev,255,(uint8*)buf16)!=MAPLE_EOK)
		return(-1);
	if(vmu_block_read(dev,buf16[0x46/2],(uint8*)buf16)!=MAPLE_EOK)
		return(-1);
	for(i=0;i<200;++i) 
		if(buf16[i]==0xfffc)
			free_blocks++;
	return(free_blocks);
}

void free_game_list(game_node_t *gptr) {
	game_node_t *aux;

	while (gptr != NULL) {
		aux=gptr->next;
		free(gptr);
		gptr=aux;
	}
}

void free_save_list(save_node_t *sptr) {
	save_node_t *aux;

	while (sptr != NULL) {
		aux=sptr->next;
		free(sptr);
		sptr=aux;
	}
}

void free_vmu_list(vmu_node_t *vptr) {
	vmu_node_t *aux;

	while (vptr != NULL) {
		aux=vptr->next;
		free(vptr);
		vptr=aux;
	}
}

void splash_screen(char *gzfile, int width, int height) {
	uint8 *buf;
	int x, y, i, j, pos_x, pos_y;
	uint8 color=0;
	gzFile f;

	pos_x=(640-width)/2;
	pos_y=(480-height)/2;
	buf=(uint8 *)malloc(width*height);
	f=gzopen(gzfile, "r");
	gzread(f, buf, width*height);
	gzclose(f);
	for (j=0; j<3; j++) {
		for (i=0; i <=255; i++)	{
			switch (j) {
				case 0: color=i;
						break;
				case 1: color=0xff;
						break;
				case 2: color=0xff-i;
						break;
			}
			for (y=0; y < height; y++) {
				for (x=0; x < width; x++) {
					if (color <= *(buf + y*width+x)) {
						draw_pixel(x+pos_x, y+pos_y, PACK_RGB565(color, color, color));
					}
				}
			}
		}
	}
	free(buf);
}

void play_s3m(char *fname) {
	void *song;
	ssize_t len;
	
	len=fs_load(fname, &song);
	spu_disable();
	spu_memload(0x10000, song, len);
	spu_memload(0, s3mplay, sizeof(s3mplay));
// DEBUG
//	printf("Load %s OK, starting ARM\n", fname);
// DEBUG
	spu_enable();
	while (*snd_dbg != 3)
		;
	while (*snd_dbg == 3)
		;
	free(song);
}

void credits_scroll() {
	int y=500;

	pvr_mem_free(back_tex);
    back_init("/rd/cred_bg.png");
//						"1234567890123456789A123456789B123456789C"
	strcpy(games_lst,	"¬          VMU Backup CD v1.0.0\n"
						"\n"
						"      Code & Gfx by Damian Parrino\n"
						"\n"
						"¬                Greetz\n"
						"\n"
						"Jeff.Ma (for many rare saves)\n"
						"PlanetWeb Site (for a lot of saves)\n"
						"Tommi Uimonen (for the music)\n"
						"Dan Potter (for KallistiOS)\n"
						"AndrewK (for DCload-ip & tool)\n"
						"Lawrence Sebald (for the MinGW guide)\n"
						"SEGA (for the greatest console!)\n"
						"Jasc Software (for Paint Shop Pro)\n"
						"\n"
						"    Copyright (C) 2005 - El Bucanero\n"
						"\n"
						"     Made with 100% recycled bytes\n"
						"\n"
						"¬      Released on 11/March/MMV\n"
						"\n\n\n\n\n\n\n\n"
						"         www.bucanero.com.ar\n");
	while (y > -800) {
	    pvr_wait_ready();
		pvr_scene_begin();
	    pvr_list_begin(PVR_LIST_OP_POLY);
		draw_back();
	    pvr_list_finish();
		pvr_list_begin(PVR_LIST_TR_POLY);
		draw_string(0, y , 3, 1, CREDITS_RGB, games_lst, 2, 2);
		pvr_list_finish();
		pvr_scene_finish();
		y--;
	}
	pvr_mem_free(back_tex);
    back_init("/rd/gameselect.png");
}

int main(int argc, char **argv) {
	int done=0;
	interface_t interface;

	cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
		(void (*)(unsigned char, long  unsigned int))arch_exit);

	vmu_set_icon(buk_icon_xpm);
	splash_screen("/rd/buk_scr.gz", 270, 215);
	vmu_set_icon(vmu_icon_xpm);
	splash_screen("/rd/vmu_scr.gz", 292, 202);

	pvr_init_defaults();

    font_init();
    back_init("/rd/gameselect.png");

	play_s3m("/rd/haiku.s3m");

	interface.games=(game_node_t *)malloc(sizeof(game_node_t));
	interface.saves=NULL;
	interface.vmus=NULL;

	interface.game_pos=(menu_pos_t *)malloc(sizeof(menu_pos_t));
	interface.game_pos->top=0;
	interface.game_pos->pos=0;
	interface.game_pos->total=load_game_list(interface.games);
	interface.game_pos->maxpage=GAMES_MAX_PAGE;

	interface.save_pos=(menu_pos_t *)malloc(sizeof(menu_pos_t));
	interface.save_pos->maxpage=SAVES_MAX_PAGE;

	interface.vmu_pos=(menu_pos_t *)malloc(sizeof(menu_pos_t));
	interface.vmu_pos->maxpage=VMU_MAX_PAGE;

	while (done != -1) {
		done=file_select(interface, interface.game_pos);
	}

	free_game_list(interface.games);
	free(interface.game_pos);
	free(interface.save_pos);
	free(interface.vmu_pos);

	pvr_mem_free(back_tex);
	pvr_mem_free(font_tex);

	return(0);
}
