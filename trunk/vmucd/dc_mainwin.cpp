// $Id$

#include <kos.h>
#include "prototypes.h"

#define VM_PIXEL_SIZE 13.0f
#define VM_PIXEL_BORDER 2.0f

int vmuscreen[48][32];
uint8 vmubitmap[192];
pvr_poly_hdr_t hdr;
pvr_stats_t stats;

void screen_init()
{
   pvr_poly_cxt_t cxt;
   
   pvr_set_bg_color(0, 111 / 255.0f, 111 / 255.0f);
   pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
   cxt.gen.shading = PVR_SHADE_FLAT;
   pvr_poly_compile(&hdr, &cxt);
   memset(vmuscreen, 0, sizeof(vmuscreen));
   memset(vmubitmap, 0, sizeof(vmubitmap));
}

void error_msg(char *fmt, ...)
{
  /*va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  fputc('\n', stderr);
  va_end(va);*/
}

void vmputpixel(int x, int y, int p) {
   if (y < 32) {
      if (1 == (p&1)) {
         vmuscreen[x][y] = 1;
      } else {
         vmuscreen[x][y] = 0;
      }
   }
}

void drawpixel(int x, int y)
{
   pvr_vertex_t vert;

   vert.flags = PVR_CMD_VERTEX;
   float actualx, actualy;

   actualx = (x * VM_PIXEL_SIZE) + 8.0f;
   actualy = (y * VM_PIXEL_SIZE) + 32.0f;

   vert.flags = PVR_CMD_VERTEX;
   vert.x = actualx;
   vert.y = actualy + VM_PIXEL_SIZE - VM_PIXEL_BORDER;
   vert.z = 10.0f;
   vert.u = 0.0f;
   vert.v = 0.0f;
   int color;
   if (vmuscreen[x][y] == 0) {
      color = 100;
   } else {
      color = 0;
   }
   vert.argb = PVR_PACK_COLOR(1.0f, (float) 0 / 255.0f, (float) color / 255.0f, (float) color / 255.0f);
   vert.oargb = 0;
   pvr_prim(&vert, sizeof(vert));

   vert.x = actualx;
   vert.y = actualy;
   pvr_prim(&vert, sizeof(vert));

   vert.x = actualx + VM_PIXEL_SIZE - VM_PIXEL_BORDER;
   vert.y = actualy + VM_PIXEL_SIZE - VM_PIXEL_BORDER;
   pvr_prim(&vert, sizeof(vert));
 
   vert.flags = PVR_CMD_VERTEX_EOL;
   vert.x = actualx + VM_PIXEL_SIZE - VM_PIXEL_BORDER;
   vert.y = actualy;
   pvr_prim(&vert, sizeof(vert));
}

void prepare_vmu_bitmap() {
   int i, j;
   uint8 tmpbyte[6];

   for (i = 0; i < 32; ++i) {
      memset(tmpbyte, 0, sizeof(tmpbyte)); 
      for (j = 0; j < 48; ++j) {
         tmpbyte[(47 - j) / 8] |= vmuscreen[47-j][31 - i] << ((47-j) % 8);
      }
      for (j = 0; j < 6; ++j) {
         vmubitmap[(i * 6) + (5 - j)] = tmpbyte[j];
	  }
   }
}

void pvr_draw_lcd() {
	int i, j;

	pvr_wait_ready();
	pvr_scene_begin();
	pvr_list_begin(PVR_LIST_OP_POLY);
	pvr_prim(&hdr, sizeof(hdr));
	for (i = 0; i < 48; ++i) {
		for (j = 0; j < 32; ++j) {
			drawpixel(i,j);
		}
	}
	pvr_list_finish();
	pvr_scene_finish();
}

void redrawlcd() {
	maple_device_t *vmu1;
	pvr_draw_lcd();
	vmu1 = maple_enum_type(0, MAPLE_FUNC_LCD);
	prepare_vmu_bitmap();
	vmu_draw_lcd(vmu1, vmubitmap);
	pvr_draw_lcd();
}

void checkevents() {
   cont_state_t *cond;
   static int old_up=0;
   static int old_dn=0;
   static int old_rt=0;
   static int old_lt=0;
   static int old_a=0;
   static int old_b=0;
   static int old_m=0;
   static int old_s=0;

   cond = (cont_state_t*)maple_dev_status(maple_enum_type(0, MAPLE_FUNC_CONTROLLER));

   if (((cond->buttons & CONT_DPAD_UP) || (cond->joyy < 0)) &&!old_up) {
      old_up=1;
	  keypress(0);
   }
   if (((cond->buttons & CONT_DPAD_DOWN) || (cond->joyy > 0)) &&!old_dn) {
      old_dn=1;
	  keypress(1);
   }
   if (((cond->buttons & CONT_DPAD_LEFT)  || (cond->joyx < 0)) &&!old_lt) {
      old_lt=1;
	  keypress(2);
   }
   if (((cond->buttons & CONT_DPAD_RIGHT) || (cond->joyx > 0)) &&!old_rt) {
	  old_rt=1;
      keypress(3);
   }
   if ((cond->buttons & CONT_A) &&!old_a) {
	  old_a=1;
      keypress(4);
   }
   if ((cond->buttons & CONT_B) &&!old_b) {
      old_b=1;
	  keypress(5);
   }
   if ((cond->buttons & CONT_X) &&!old_s) {
	  old_s=1;
      keypress(6);
   }
   if ((cond->buttons & CONT_Y) &&!old_m) {
      old_m=1;
	  keypress(7);
   }
   if (!((cond->buttons & CONT_DPAD_UP)  || (cond->joyy < 0)) &&old_up) {
	  old_up=0;
      keyrelease(0);
   }
   if (!((cond->buttons & CONT_DPAD_DOWN) || (cond->joyy > 0)) &&old_dn) {
      old_dn=0;
	  keyrelease(1);
   }
   if (!((cond->buttons & CONT_DPAD_LEFT) || (cond->joyx < 0)) &&old_lt) {
      old_lt=0;
	  keyrelease(2);
   }
   if (!((cond->buttons & CONT_DPAD_RIGHT) || (cond->joyx > 0)) &&old_rt) {
      old_rt=0;
	  keyrelease(3);
   }
   if (!(cond->buttons & CONT_A) &&old_a) {
      old_a=0;
	  keyrelease(4);
   }
   if (!(cond->buttons & CONT_B) &&old_b) {
	  old_b=0;
      keyrelease(5);
   }
   if (!(cond->buttons & CONT_X) &&old_s) {
      old_s=0;
	  keyrelease(6);
   }
   if (!(cond->buttons & CONT_Y) &&old_m) {
      old_m=0;
	  keyrelease(7);
   }
}

void waitforevents(uint32 t) {
   timer_spin_sleep(t);
   return;
}
