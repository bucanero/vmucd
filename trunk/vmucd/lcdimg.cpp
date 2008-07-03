// $Id$

#include <kos.h>
#include <time.h>
#include <errno.h>
#include "prototypes.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int lcdi_width, lcdi_height, lcdi_nframes;
static long *lcdi_frametime;
static unsigned char **lcdi_framedata;

static void display_frame(int f)
{
  unsigned char *p = lcdi_framedata[f];
  int x, y, w = lcdi_width, h = lcdi_height;
  if(w>48)
    w = 48;
  if(h>32)
    h = 32;
  for(y=0; y<h; y++) {
    unsigned char *p2 = p;
    for(x=0; x<w; x++)
      vmputpixel(x, y, !!*p2++);
    p += lcdi_width;
  }
  redrawlcd();
}

static void free_framedata()
{
  int i;
  for(i=0; i<lcdi_nframes; i++)
    if(lcdi_framedata[i] != NULL)
      free(lcdi_framedata[i]);
  free(lcdi_framedata);
}

static int myread(int fd, unsigned char *data, int len)
{
  int r, rtot=0;
  while(len>0) {
    r = fs_read(fd, data, len);
    if(r == 0 || r >= len)
      return r+rtot;
    if(r < 0) {
      if(errno == EAGAIN)
	timer_spin_sleep(1000);
      else if(errno != EINTR)
	return -1;
    } else {
      data += r;
      len -= r;
      rtot += r;
    }
  }
  return rtot;
}

int initimg(int fd)
{
  unsigned char hdr[16], tm[4];
  int i, fsz;
  if(myread(fd, hdr, 16)<16)
    return 0;
  if(strncmp((char*)hdr, "LCDi", 4))
    return 0;
  lcdi_width = hdr[6]+(hdr[7]<<8);
  lcdi_height = hdr[8]+(hdr[9]<<8);
  lcdi_nframes = hdr[14]+(hdr[15]<<8);
  fsz = lcdi_width * lcdi_height;
  if((lcdi_frametime = (long*)calloc(lcdi_nframes, sizeof(long)))==NULL)
    return 0;
  for(i=0; i<lcdi_nframes; i++) {
    if(myread(fd, tm, 4)<4) {
      free(lcdi_frametime);
      return 0;
    }
    lcdi_frametime[i] = tm[0]+(tm[1]<<8)+(tm[2]<<16)+(tm[3]<<24);
  }
  if((lcdi_framedata = (unsigned char**)calloc(lcdi_nframes, sizeof(unsigned char *)))==NULL) {
    free(lcdi_frametime);
    return 0;
  }
  for(i=0; i<lcdi_nframes; i++)
    lcdi_framedata[i] = NULL;
  for(i=0; i<lcdi_nframes; i++)
    if((lcdi_framedata[i] = (unsigned char*)malloc(fsz))==NULL ||
       myread(fd, lcdi_framedata[i], fsz)<fsz) {
      free_framedata();
      free(lcdi_frametime);
      return 0;
    }
  return 1;
}

void freeimg()
{
  free_framedata();
  free(lcdi_frametime);  
}

int loadimg(char *filename)
{
  int fd;

  if((fd = fs_open(filename, O_RDONLY|O_BINARY))>=0) {
    if(!initimg(fd)) {
      fs_close(fd);
      error_msg("%s: can't decode image", filename);
      return 0;
    }
    fs_close(fd);
    return 1;
  } else {
    /*perror(filename);*/
    return 0;
  }
}

int do_lcdimg(char *filename)
{
  extern unsigned char sfr[0x100];

  if(!loadimg(filename))
    return 0;
  sfr[0x4c] = 0xff;
  if(lcdi_nframes>1) {
    struct timeval tnow, tnext, tdel;
    int fcntr=-1;
    gettimeofday(&tnext, NULL);
    for(;;) {
      gettimeofday(&tnow, NULL);
      if(tnow.tv_sec > tnext.tv_sec ||
	 (tnow.tv_sec == tnext.tv_sec && tnow.tv_usec >= tnext.tv_usec)) {
	if((++fcntr>=lcdi_nframes))
	  fcntr=0;
	display_frame(fcntr);
	tnext.tv_sec += lcdi_frametime[fcntr]/1000;
	if((tnext.tv_usec += (lcdi_frametime[fcntr]%1000)*1000)>=1000000) {
	  tnext.tv_sec++;
	  tnext.tv_usec -= 1000000;
	}
      } else {
        uint32 t;
        t = (tnext.tv_sec - tnow.tv_sec) * 1000;
            
	/*tdel.tv_sec = tnext.tv_sec - tnow.tv_sec;*/
	if((tdel.tv_usec = tnext.tv_usec - tnow.tv_usec)<0) {
          t -= 1000;
          t += (tnext.tv_usec - tnow.tv_usec) / 1000;

/*	  tdel.tv_sec--;
	  tdel.tv_usec += 1000000;*/
	}
	waitforevents(t);
      }
      checkevents();
      if((sfr[0x4c]&0x70)!=0x70)
	break;
    }
  } else if(lcdi_nframes==1) {
    display_frame(0);
    for(;;) {
      /* waitforevents(NULL); Useless call & return function */
      checkevents();
      if((sfr[0x4c]&0x70)!=0x70)
	break;
    }
  }
  freeimg();
  return 1;
}
