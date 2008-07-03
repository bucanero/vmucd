// $Id$
/********************************************************************
 * VMU Backup CD v1.3.0 (Aug/2005)
 * screen.h - coded by El Bucanero
 *
 * Copyright (C) 2005 Damian Parrino <bucanero@fibertel.com.ar>
 * http://www.bucanero.com.ar/
 *
 ********************************************************************/

#define VMU_X 510
#define VMU_Y 210
#define VMU_RGB 0, 0, 0
#define GAMES_X 30
#define GAMES_Y 35
#define GAMES_RGB 0, 0, 0
#define SAVES_X 515
#define SAVES_Y 35
#define SAVES_RGB 0, 0, 0
#define INFO_X 30
#define INFO_Y 360
#define INFO_LINE_LENGTH 72
#define INFO_RGB 0, 0, 0
#define ICON_X 30
#define ICON_Y 360
#define CREDITS_RGB 1, 1, 1

#define draw_pixel(x, y, color) vram_s[(y)*640+(x)]=(color)
#define PACK_RGB565(r, g, b) (((r>>3)<<11)|((g>>2)<<5)|((b>>3)<<0))

//void draw_frame(void);
void back_init(char *txrfile);
void font_init(char *gzfile);
void draw_back(void);
void draw_char(float x1, float y1, float z1, float a, float r, float g, float b, int c, float xs, float ys);
void draw_string(float x, float y, float z, float a, float r, float g, float b, char *str, float xs, float ys, int max_len=0);
void draw_mono_icon(int pos_x, int pos_y, uint8 *icon);
void draw_color_icon(int pos_x, int pos_y, uint16 *pal, uint8 *icon);
void splash_screen(char *gzfile, int width, int height);
void credits_scroll(char *credits_bg, char *original_bg);

pvr_ptr_t font_tex;
pvr_ptr_t back_tex;

void back_init(char *txrfile) {
    back_tex = pvr_mem_malloc(512*512*2);
    png_to_texture(txrfile, back_tex, PNG_NO_ALPHA);
}

void font_init(char *gzfile) {
    uint8 *temp_tex;
	gzFile f;
    
    font_tex = pvr_mem_malloc(256*256*2);
	temp_tex=(uint8 *)malloc(512*128);
	f=gzopen(gzfile, "r");
	gzread(f, temp_tex, 512*128);
	gzclose(f);
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
	uint8 sr, sg, sb;
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
		sr=(uint8)*str; str++;
		sg=(uint8)*str; str++;
		sb=(uint8)*str; str++;
		while (*str != '\n') {
			draw_char(x, y, z, a, sr/255.0, sg/255.0, sb/255.0, *str++, xs, ys);
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
	if (vmu_icon != NULL) {
		draw_color_icon(ICON_X, ICON_Y, vmu_icon->pal, vmu_icon->icon);
	}
}

/*		if (strcmp(vmu_icon->name, "ICONDATA_VMS") == 0) {
			draw_mono_icon(ICON_X, ICON_Y, vmu_icon->icon);
		} else {
			draw_color_icon(ICON_X, ICON_Y, vmu_icon->pal, vmu_icon->icon);
		}
*/

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

void credits_scroll(char *credits_bg, char *original_bg) {
	int y=500;

	pvr_mem_free(back_tex);
    back_init(credits_bg);
//						"1234567890123456789A123456789B123456789C"
	strcpy(games_lst,	"¬\xFF\xFF\x01          VMU Backup CD v1.3.0\n"
						"\n"
						"          Coded by El Bucanero\n"
						"\n"
						"  SoftVMS emulator by Marcus Comstedt\n"
						"    KallistiOS port by DirtySanchez\n"
						"\n"
						"¬\xFF\x01\x01                Greetz\n"
						"\n"
						"PlanetWeb Site (for a lot of saves)\n"
						"Jeff.Ma (for many rare saves)\n"
						"Tommi Uimonen (for the music)\n"
						"Dan Potter (for KallistiOS)\n"
						"AndrewK (for DCload-ip & tool)\n"
						"Lawrence Sebald (for the MinGW guide)\n"
						"Pavlik (for some other saves)\n"
						"\n"
						"           Special thanks to\n"
						"¬\x04\x81\xC4                 SEGA\n"
						"    - for the greatest console! -\n"
						"\n"
						"  Copyright (C) 2005 - Damian Parrino\n"
						"\n"
						"     Made with 100% recycled bytes\n"
						"\n"
						"This soft is FREEWARE - ¬\xFF\x01\x01NOT FOR RESALE!\n"
						"\n"
						"¬\xFF\xFF\x01       Released on 11/Aug/MMV\n"
						"\n\n\n\n\n\n\n\n"
						"         www.bucanero.com.ar\n");
	while (y > -1070) {
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
    back_init(original_bg);
}

void draw_mono_icon(int pos_x, int pos_y, uint8 *icon) {
	int x, y;

	for (y=0; y<32; y++) {
		for (x=0; x<32; x++) {
			if (icon[(y*32+x)/8] & (0x80 >> (x&7))) {
				draw_pixel(pos_x + x, pos_y + y, 0);
			} else {
				draw_pixel(pos_x + x, pos_y + y, 0xfff1);
			}
		}
	}
}

void draw_color_icon(int pos_x, int pos_y, uint16 *pal, uint8 *icon) {
	int x, y, nyb;
	uint16 rgb565;

	for (y=0; y<32; y++) {
		for (x=0; x<32; x+=2) {
			nyb = (icon[y*16 + x/2] & 0xf0) >> 4;
			rgb565 = ((((pal[nyb] & 0x0f00)>>8)*2)<<11) + ((((pal[nyb] & 0x00f0)>>4)*4)<<5) + ((((pal[nyb] & 0x000f)>>0)*2)<<0);
			draw_pixel(pos_x + x, pos_y + y, rgb565);

			nyb = (icon[y*16 + x/2] & 0x0f) >> 0;
			rgb565 = ((((pal[nyb] & 0x0f00)>>8)*2)<<11) + ((((pal[nyb] & 0x00f0)>>4)*4)<<5) + ((((pal[nyb] & 0x000f)>>0)*2)<<0);
			draw_pixel(pos_x + x + 1, pos_y + y, rgb565);
		}
	}
}
