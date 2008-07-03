// $Id$
/********************************************************************
 * VMU Backup CD v1.3.0 (Aug/2005)
 * vmucd.cpp - coded by El Bucanero
 *
 * VMU emulator SoftVMS by Marcus Comstedt							<http://mc.pp.se/dc/>
 * SoftVMS port to KallistiOS by DirtySanchez
 *
 * some misc code based on <kos/examples/dreamcast/png/> from KallistiOS
 *
 * Copyright (C) 2005 Damian Parrino <bucanero@fibertel.com.ar>
 * http://www.bucanero.com.ar/
 *
 * Greetz to:
 * Dan Potter for KallistiOS and other great libs like Tsunami		<http://gamedev.allusion.net/>
 * AndrewK / Napalm 2001 for the lovely DC load-ip/tool				<http://adk.napalm-x.com/>
 * Lawrence Sebald for the MinGW/msys cross compiler tutorial		<http://ljsdcdev.sunsite.dk/>
 * Dreamcast@PlanetWeb for the whole bunch of DC saves				<http://dreamcast.planetweb.com/>
 * Jeff.Ma for sharing a lot of JAP & other rare saves				<http://www.dreamcastcn.com/>
 *
 * and last, but not least, thanks to SEGA for my ALWAYS LOVED Dreamcast! =)
 *
 ********************************************************************/

#include <kos.h>
#include <png/png.h>
#include <zlib/zlib.h>
#include "prototypes.h"
#include "lists.h"
#include "vmu_icon.h"
#include "buk_icon.h"
#include "s3mplay.h"
#include "screen.h"

// GUI constants
#define VMU_MAX_PAGE 3
#define GAMES_LINE_LENGTH 54
#define GAMES_MAX_PAGE 15
#define SAVES_MAX_PAGE 7
#define SELECTED_RGB "¬\x01\x01\xFF"

int file_select(interface_t inter, menu_pos_t *menu);
void update_lists(interface_t inter);
void copy_file(interface_t inter);
void play_vmu_game(interface_t inter);

extern uint8 romdisk[];

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

int file_select(interface_t inter, menu_pos_t *menu) {
	int done=0;
	uint64	timer = timer_ms_gettime64();

	while (!done) {
		update_lists(inter);
		draw_frame();
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, t)
			if (((t->buttons & CONT_DPAD_UP) || (t->joyy < 0)) && (menu->pos >= menu->top) && (timer + 200 < timer_ms_gettime64())) {
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
			if (((t->buttons & CONT_DPAD_DOWN) || (t->joyy > 0)) && (menu->pos <= menu->top + menu->maxpage) && (timer + 200 < timer_ms_gettime64())) {
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
						if ((get_save_ptr(inter.saves, inter.save_pos->pos))->vmugame) {
							play_vmu_game(inter);
						} else {
							inter.vmu_pos->top=0;
							inter.vmu_pos->pos=0;
							inter.vmus=(vmu_node_t *)malloc(sizeof(vmu_node_t));
							inter.vmu_pos->total=load_vmu_list(inter.vmus);
							file_select(inter, inter.vmu_pos);
							free_vmu_list(inter.vmus);
							inter.vmus=NULL;
						}
					} else {
						copy_file(inter);
						done=1;
					}
				}
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_B) && (timer + 200 < timer_ms_gettime64())) {
				done=1;
// DEBUG
//				menu->pos=-1;
// DEBUG
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_Y) && (timer + 200 < timer_ms_gettime64())) {
				credits_scroll("/rd/cred_bg.png", "/rd/gameselect.png");
				timer = timer_ms_gettime64();
			}
		MAPLE_FOREACH_END()
	}
	return(menu->pos);
}

/*			if ((t->buttons & CONT_X) && (timer + 200 < timer_ms_gettime64())) {
				if (inter.saves != NULL) {
					if ((get_save_ptr(inter.saves, inter.save_pos->pos))->vmugame) {
						play_vmu_game(inter);
					}
				}
				timer = timer_ms_gettime64();
			}
*/

void update_lists(interface_t inter) {
	int		i=0;
	game_node_t	*gaux=inter.games;
	save_node_t	*saux=inter.saves;
	vmu_node_t *vaux=inter.vmus;

	strcpy(games_lst, "");
	while ((gaux->next != NULL) && (i < inter.game_pos->top + inter.game_pos->maxpage)) {
		if (i >= inter.game_pos->top) {
			if (i == inter.game_pos->pos) strcat(games_lst, SELECTED_RGB);
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
				if (i == inter.vmu_pos->pos) strcat(vmus_lst, SELECTED_RGB);
				sprintf(vmu_info, "VMU %c%c (%d)\n", vaux->name[0]-32, vaux->name[1], vaux->free_blocks);
				strcat(vmus_lst, vmu_info);
			}
			i++;
			vaux=vaux->next;
		}
	}
	strcpy(saves_lst, "");
	strcpy(vmu_info, "");
	vmu_icon=NULL;
	if (saux != NULL) {
		i=0;
		while ((saux->next != NULL) && (i < inter.save_pos->top + inter.save_pos->maxpage)) {
			if (i >= inter.save_pos->top) {
				if (i == inter.save_pos->pos) {
					strcat(saves_lst, SELECTED_RGB);
					sprintf(vmu_info, "     File: %s - %d Block(s)\n     %s", saux->name, (saux->size / 512), saux->desc);
					vmu_icon=saux;
				}
				strcat(saves_lst, saux->file);
				strcat(saves_lst, "\n");
			}
			i++;
			saux=saux->next;
		}
	}
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
			sprintf(vmu_info, "     %s: %d blocks copied to VMU %c%c.\n", sptr->name, sptr->size/512, vptr->name[0]-32, vptr->name[1]);
		} else {
			sprintf(vmu_info, "     Copy Error!\n");
		}
	} else {
		sprintf(vmu_info, "     Error! Not enough free blocks.\n");
	}
	timer = timer_ms_gettime64();
	while (timer + 1500 > timer_ms_gettime64()) {
		draw_frame();
	}
	vmu_set_icon(vmu_icon_xpm);
}

void play_vmu_game(interface_t inter) {
	save_node_t *sptr;
	char vmsfile[64];
	char hdr[4];
	file_t fd;

	sptr=get_save_ptr(inter.saves, inter.save_pos->pos);
	sprintf(vmsfile, "/cd/%s/%s", get_game_directory(inter.games, inter.game_pos->pos), sptr->file);
	fd = fs_open(vmsfile, O_RDONLY);
	fs_read(fd, hdr, 4);
	fs_close(fd);
	if(hdr[0]=='L' && hdr[1]=='C' && hdr[2]=='D' && hdr[3]=='i') {
//		printf("Loading LCD image %s\n", vmsfile);
		do_lcdimg(vmsfile);
	} else {
//		printf("Loading VMS Game %s\n", vmsfile);
		do_vmsgame(vmsfile, NULL);
	}
	vmu_set_icon(vmu_icon_xpm);
//   halt_mode();
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

    font_init("/rd/font.gz");
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
