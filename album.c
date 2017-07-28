#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include "jpeg.h"
#include <errno.h>
#include <dirent.h>

#define LIST_NODE_NUM_MAX 10
#define CURRENT 0 
#define PREV    1 
#define NEXT    2 

typedef struct node
{
	char *data;
	struct node *next;
	struct node *prev;
}linknode, *linklist;

linklist *init_list(int data)
{
	linklist head = malloc(sizeof(linknode));
	if(head != NULL)
	{
		head->prev = head->next = head;
		head->data = NULL;
	}
	return head;
}

linklist newnode(char *name)
{
	linklist new = malloc(sizeof(linknode));
	if(new != NULL)
	{
		new->data = NULL;
		new->prev = new->next = NULL;
	}

	return new;
}

void list_add(linklist new, linklist head)
{
		if(head == NULL)
			return new;

		head->prev->next =  new;
		head->prev = new;

		new->prev = head->prev;
		new->next = head;
}

int wait4touch(int ts);
{
	struct input_event buf;
	bzero(&buf, sizeof(buf));

	int info = 0;
	while(1)
	{
		read(ts, &buf, sizeof(buf));

		if(buf.type == EV_ABS)
		{
			
			if(buf.code == ABS_X)
			{
					info = buf.value;		
			}
		}

		if(buf.type == EV_KEY)
		{
			
			if(buf.code == BTN_TOUCH && buf.value == 0)
					break;
		}

	}
	if(info > 400)
	{
			
	}



}

bool is_jpeg(char *name)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	dinfo.err = jpeg_std_error(&jerr);
	jpeg_mem_src(&cinfo, jpg_buffer, file_info.st_size);

	int ret = jpeg_read_header(&cinfo, true);
	if(ret != 1)
	{
		fprintf(stderr, "jpeg_read_header failed: %s\n", sterror(errno));
		exit(1);
	}

	jpeg_start_decompress(&cinfo);
}

int show_jpeg(linklist head, int action)
{
	char *data = malloc(50);
	linklist node = newnode(eq->d_name);

}

int main(int argc, char **argv) // ./album  jpg/ ,  ./album  ,  ./album 1.jpg
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
	linklist head = init_list(LIST_NODE_NUM_MAX);

	if(S_ISDIR(fileinfo.st_mode))
	{
		DIR *dp = opendir(target);
		chdir(target);

		// 将指定目录中的jpg/jpeg图片妥善地保存起来
		struct dirent *ep;
		while(1)
		{
			ep = readdir(dp);

			if(ep == NULL)
				break;

			bzero(&fileinfo, sizeof(fileinfo));
			stat(ep->d_name, &fileinfo);

			// 遇到一个非普通文件
			if(!S_ISREG(fileinfo.st_mode))
				continue;

			// 遇到一个非jpg/jpeg的普通文件，直接跳过
			if(!is_jpeg(ep->d_name))
				cotninue;

			// 现在就是一张jpg/jpeg图片
			// 将这张图片的相关信息、图像数据插入链表
			// newnode() 负责将图片的名字放入节点
			linklist new = newnode(ep->d_name);
			list_add(new, head);
		}



		// 准备好触摸屏
		int ts = open("/dev/input/event0", O_RDONLY);
		if(ts == -1)
		{
			perror("open /dev/input/event0 failed");
			exit(0);
		}

		// 使用触摸屏来回浏览这个链表中的图片，形成一个简单相册
		linklist jpg = head;
		int action = CURRENT;
		while(1)
		{
			// 将jpg对应的图片显示出来
			// action: CURRENT 显示jpg指向的当前图片
			//         PREV    显示jpg指向的上一张图片
			//         NEXT    显示jpg指向的下一张图片
			jpg = show_jpeg(jpg, action);

			// 等待手指的触碰，并得到上/下动作指令
			action = wait4touch(ts);
		}
	}

	// 直接显示一张jpg/jpeg图片
	else if(S_ISREG(fileinfo.st_mode))
	{
		if(is_jpg(target))
		{
			show_jpeg(target);
		}
		else
		{
			printf("NOT a jpg/jpeg file\n");
		}
	}

	return 0;
}


