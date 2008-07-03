// $Id$

#define GETTIMEOFDAY(tp) gettimeofday(tp, NULL)
#define EAGAIN 11
#define EINTR 4
#define COLOR(r, g, b)  (((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | (((b >> 3) & 0x1f) << 0)

extern void waitforevents(uint32 t);
extern void halt_mode();
extern void checkevents();
extern void redrawlcd();
extern void vmputpixel(int, int, int);
extern void keypress(int i);
extern void keyrelease(int i);
extern int do_vmsgame(char *filename, char *biosname);
extern int do_lcdimg(char *filename);
extern void error_msg(char *fmt, ...);
extern void screen_init();
