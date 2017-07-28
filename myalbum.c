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

void read_image_from_file(int fd, unsigned char *jpg_buffer, unsigned long image_size)
{
	int nread = 0;
	int size = image_size;
	unsigned char *buf = jpg_buffer;

	// 循环地将jpeg文件中的所有数据，统统读取到jpg_buffer中
	while(size > 0)
	{
		nread = read(fd, buf, size);
		if(nread == -1)
		{
			printf("read jpg failed \n");
			exit(0);
		}
		size -= nread;
		buf +=nread;
	}
}

void write_lcd(unsigned char *FB, struct fb_var_screeninfo *vinfo, unsigned char *rgb_buffer, struct image_info *imageinfo)
{
	int x, y;
	for(y=0; y<imageinfo->height && y<vinfo->yres; y++)
	{
		for(x=0; x<imageinfo->width && x<vinfo->xres; x++)
		{
			int image_offset = x * 3 + y * imageinfo->width * 3;
			int lcd_offset   = x * 4 + y * vinfo->xres * 4;

			memcpy(FB+lcd_offset, rgb_buffer+image_offset+2, 1);
			memcpy(FB+lcd_offset+1, rgb_buffer+image_offset+1, 1);
			memcpy(FB+lcd_offset+2, rgb_buffer+image_offset, 1);
		}
	}
}

linklist init_list(void)
{
	linklist head = malloc(sizeof(listnode));

	if(head != NULL)
	{
		head->next = head;
		head->prev = head;
	}

	return head;
}

linklist newnode(char *name)
{
	linklist new = malloc(sizeof(listnode));

	if(new != NULL)
	{
		new->data = name;
		new->next = NULL;
		new->prev = NULL;
	}

	return new;
}

bool list_add_tail(linklist new, linklist head)
{
	if(new == NULL)
		return false;

	new->prev = head->prev;  //新节点前指针指向头结点前一个结点
	new->next = head->next;  //新节点后指针指向头结点后一个结点

	head->prev->next = new;  
	head->next->prev = new;
	head->prev = new;		//头结点前向指针同时也指向新节点
	                       //刷图过程把头结点跳过，头结点只起到定位某个结点作用
	return true;
}

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

int show_jpeg(char *jpeg_name)   //功能：显示图片名字为***的jpg图片到LCD屏上
{
	// 获取文件的属性
	struct stat file_info;
	stat(jpeg_name, &file_info);

	// 打开jpeg文件
	int fd = open(jpeg_name, O_RDWR);
	if(fd == -1)
	{
		printf("open the argv[1] failed\n");
	}

	// 根据获取的stat信息中的文件大小，来申请一块恰当的内存，用来存放jpeg编码的数据
	unsigned char *jpg_buffer = calloc(1, file_info.st_size);
	read_image_from_file(fd, jpg_buffer, file_info.st_size);


	/********* 以下代码，就是使用了jpeglib的函数接口，来将jpeg数据解码 *********/

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpg_buffer, file_info.st_size);

	int ret = jpeg_read_header(&cinfo, true);
	if(ret != 1)
	{
		fprintf(stderr, "jpeg_read_header failed: %s\n", strerror(errno));
		exit(1);
	}

	jpeg_start_decompress(&cinfo);

	/////////  在解码jpeg数据的同时，顺便将图像的尺寸信息保留下来
	struct image_info *image_info = calloc(1, sizeof(struct image_info));
	if(image_info == NULL)
	{
		printf("malloc image_info failed\n");
	}	

	image_info->width = cinfo.output_width;
	image_info->height = cinfo.output_height;
	image_info->pixel_size = cinfo.output_components;
	/////////

	int row_stride = image_info->width * image_info->pixel_size;

	unsigned long rgb_size;
	unsigned char *rgb_buffer;
	rgb_size = image_info->width * image_info->height * image_info->pixel_size;

	rgb_buffer = calloc(1, rgb_size);

	int line = 0;
	while(cinfo.output_scanline < cinfo.output_height)
	{
		unsigned char *buffer_array[1];
		buffer_array[0] = rgb_buffer + (cinfo.output_scanline) * row_stride;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(jpg_buffer);
	close(fd);

	/****************************************************************************/

	int lcd = open("/dev/fb0", O_RDWR);
	if(lcd == -1)
	{
		printf("open the lcd failed %s", strerror(errno));
		exit(1);
	}

	struct fb_var_screeninfo vinfo;
	ioctl(lcd, FBIOGET_VSCREENINFO, &vinfo);

	unsigned long bpp = vinfo.bits_per_pixel;
	unsigned char *fbmem = mmap(NULL, vinfo.xres * vinfo.yres * bpp/8,
				    PROT_READ|PROT_WRITE, MAP_SHARED, lcd, 0);
	if(fbmem == MAP_FAILED)
	{
		perror("mmap() failed");
		exit(0);
	}

	write_lcd(fbmem, &vinfo, rgb_buffer, image_info);

	munmap(fbmem, vinfo.xres * vinfo.yres * bpp/8);
	close(lcd);
	free(image_info);
	free(rgb_buffer);
	return 0;
}

bool is_jpeg(char *name)    //功能：判断名字为jpeg_name的文件是否为jpg图片，是返回真，不是返回假。
{
	if(strstr(name, ".jpg") == NULL && strstr(name, ".jpeg") == NULL)
		return false;

	struct stat file_info;
	stat(name, &file_info);

	// 打开jpeg文件
	int fd = open(name, O_RDWR);
	if(fd == -1)
	{
		printf("open the argv[1] failed\n");
	}

	// 根据获取的stat信息中的文件大小，来申请一块恰当的内存，用来存放jpeg编码的数据
	unsigned char *jpg_buffer = calloc(1, file_info.st_size);
	read_image_from_file(fd, jpg_buffer, file_info.st_size);


	/********* 以下代码，就是使用了jpeglib的函数接口，来将jpeg数据解码 *********/

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpg_buffer, file_info.st_size);

	int ret = jpeg_read_header(&cinfo, true);
	if(ret != 1)
	{
		free(jpg_buffer);
		close(fd);
		return false;
	}
	else
	{
		free(jpg_buffer);
		close(fd);
		return true;
	}

}

linklist show_pic(linklist jpg, int action)
{
	if(jpg == NULL)
	{
		printf("show_jpg failed");
		return NULL;
	}

	switch(action)    //根据action值判断当前的情况
	{
		case CURRENT:
			printf("%s\n", jpg->data);  //用char *data指针指向图片名字
			show_jpeg(jpg->data);       //名字再指向jpeg图片数据
			return jpg;                 //把名字传给show_jpeg函数显示出图片
		case NEXT:
			printf("%s\n", jpg->next->data);
			show_jpeg(jpg->next->data);
			return jpg->next;
		case PREV:
			printf("%s\n", jpg->prev->data);
			show_jpeg(jpg->prev->data);
			return jpg->prev;
		default:
			printf("unknow! error\n");
			break;
	}

	return jpg;
}

struct input_event buf;
struct stat fileinfo;

int main(int argc, char **argv) // ./album  jpg/ ,  ./album  ,  ./album 1.jpg
{
	if(argc > 2)
	{
		printf("invalid argument\n");
		exit(0);
	}

	char *target = argc==2 ? argv[1] : ".";

	struct stat fileinfo;
	bzero(&fileinfo, sizeof(fileinfo));
	stat(target, &fileinfo);

	// 准备好一条双向循环链表
	// 这是一条图片的缓存链表，最多缓存10张图片的数据
	linklist head = init_list();

	if(S_ISDIR(fileinfo.st_mode))
	{
		DIR *dp = opendir(target);
		if(dp == NULL)
		{
			printf("open target is empty!\n");
		}
		chdir(target);

		// 将指定目录中的jpg/jpeg图片妥善地保存起来
		struct dirent *ep = malloc(sizeof(struct dirent));
		while(1)
		{
			errno = 0;
			ep = readdir(dp);

			if(ep == NULL && errno != 0)
			{
				perror("open dirent ep error\n");
				exit(0);
			}
			if(ep == NULL)
			{
				free(ep);
				break;
			}

			bzero(&fileinfo, sizeof(fileinfo));
			stat(ep->d_name, &fileinfo);

			// 遇到一个非普通文件
			if(!S_ISREG(fileinfo.st_mode))
				continue;

			// 遇到一个非jpg/jpeg的普通文件，直接跳过
			if(!is_jpeg(ep->d_name))
				continue;

			// 现在就是一张jpg/jpeg图片
			// 将这张图片的相关信息、图像数据插入链表
			// newnode() 负责将图片的名字放入节点
			linklist new = newnode(ep->d_name);
			list_add_tail(new, head);
		}

		// 准备好触摸屏
		int ts = open("/dev/input/event0", O_RDONLY);
		if(ts == -1)
		{
			perror("open /dev/input/event0 failed");
			exit(0);
		}

		// 使用触摸屏来回浏览这个链表中的图片，形成一个简单相册
		linklist jpg = head->next;
		int action = CURRENT;
		bool flag = false;
		while(1)
		{
			// 将jpg对应的图片显示出来
			// action: CURRENT 显示jpg指向的当前图片
			//         PREV    显示jpg指向的上一张图片
			//         NEXT    显示jpg指向的下一张图片
			if(flag)
			{
				break;
			}

			jpg = show_pic(jpg, action);

			// 等待手指的触碰，并得到上/下动作指令
			struct input_event buf;
			bzero(&buf, sizeof(buf));

			while(1)
			{
				read(ts, &buf, sizeof(buf));

				if(buf.type == EV_ABS)
				{                   //当按键按到屏幕左半部分，屏幕显示上一张图片
					if(buf.code == ABS_X && buf.value >= 0 && buf.value <= 400)
					{
						printf("%d\n", buf.value);
						action = PREV;
					}               //当按键按到屏幕右半部分，屏幕显示下一张图片
					if(buf.code == ABS_X && buf.value >= 400 && buf.value <= 800)
					{
						printf("%d\n", buf.value);
						action = NEXT;
					}              //当按键按到y轴方向顶部0~50行时，程序退出
					if(buf.code == ABS_Y && buf.value >=0 && buf.value <= 50)
						flag = true;
				}

				if(buf.type == EV_KEY)
				{                  //按键松手则跳出循环
					if(buf.code == BTN_TOUCH && buf.value == 0)
						break;
				}
			}
		}

		close(ts);
		closedir(dp);
	}

	// 直接显示一张jpg/jpeg图片
	else if(S_ISREG(fileinfo.st_mode))
	{
		if(is_jpeg(target))
		{
			linklist new = newnode(target);
			show_pic(new, CURRENT);
			free(new);
		}
		else
		{
			printf("NOT a jpg/jpeg file\n");
		}
	}

	return 0;
}
