#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"


/* 定义按键值 */
#define KEY0VALUE	0XF0
#define INVAKEY		0X00


/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, fl, ret,i;
	char *keyfilename,*ledfilename;
	int keyvalue, ledvalue=0;
	unsigned char ledbuff[1];
	
	if(argc != 3){
		printf("Error Usage!\r\n");
		return -1;
	}

	keyfilename = argv[1];
	ledfilename = argv[2];

	/* 打开key驱动 */
	fd = open(keyfilename, O_RDWR);
	if(fd < 0){
		printf("file %s open failed!\r\n", argv[1]);
		return -1;
	}

	fl = open(ledfilename, O_RDWR);
	if(fl < 0){
		printf("file %s open failed!\r\n", argv[2]);
		return -1;
	}

	/* 循环读取按键值数据！ */
	while(1) {
		read(fd, &keyvalue, sizeof(keyvalue));
		i=0;
		while (++i < 100000);
		if (keyvalue == KEY0VALUE) {	/* KEY0 */
			printf("KEY0 Press will overturn led\r\n");	/* 按下 */

			read(fl, &ledvalue, sizeof(ledvalue));
			printf("read value:%d\r\n",ledvalue);

			ledbuff[0] = (unsigned char)ledvalue;
			ret = write(fl,ledbuff,sizeof(ledbuff));
			if(ret < 0){
				printf("LED Control Failed!\r\n");
				close(fd);
				close(fl);
				return -1;
			}
		}

	}

	ret= close(fd); /* 关闭文件 */
	if(ret < 0){
		printf("file %s close failed!\r\n", argv[1]);
		return -1;
	}
	return 0;
}
