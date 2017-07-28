#ifndef __HEAD_H_
#define __HEAD_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <errno.h>
#include <linux/input.h>

#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <signal.h>
#include <linux/fb.h>
#include <string.h>
#include "jpeglib.h"

#define DATATYPE char *
#define CURRENT 0 
#define PREV    1 
#define NEXT    2 

#define RED   2
#define GREEN 1
#define BLUE  0

typedef DATATYPE datatype;

typedef struct node
{
	datatype data;
	struct node *prev;
	struct node *next;
}listnode, *linklist;

struct image_info
{
	int width;
	int height;
	int pixel_size;
};

struct image_info_bmp
{
	int height;
	int width;
	int bpp;
};

struct header
{
	int16_t type;
	int32_t size;
	int16_t reserved1;
	int16_t reserved2;
	int32_t offbits;
}__attribute__((packed));

struct info
{
	int32_t size;
	int32_t width;
	int32_t height;
	int16_t planes;

	int16_t bit_count;
	int32_t compression;
	int32_t size_img;
	int32_t X_pel;
	int32_t Y_pel;
	int32_t clrused;
	int32_t clrImportant;
}__attribute__((packed));

void display_bmp(unsigned char *fbmemy, struct fb_var_screeninfo *pvinfo, unsigned char *imgdata,
		struct image_info_bmp *pimgfo, int xoffset, int yoffset);

unsigned char *load_bmp(char *bmpfile, struct image_info_bmp *pimgfo);

#endif

/*bool list_del(linklist del, linklist head)
{
	if(head == NULL)
		return false;

	del->prev->next = del->next;
	del->next->prev = del->prev;

	del->prev = del->next = NULL;
	return true;
}
*/


/*xoffset = ((int)pvinfo->xres - pimgfo->width)/2;
yoffset = ((int)pvinfo->yres - pimgfo->height)/2;

xoffset = xoffset >= 0 ? xoffset : 0;
yoffset = yoffset >= 0 ? yoffset :0;*/

