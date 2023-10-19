#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include "linux/ioctl.h"

int main(int argc, char *argv[])
{
    int fd, ret = 0;
    char *filename;
    struct pollfd fds;
    fd_set readfds;
    struct timeval timeout;
    unsigned char data;

    if (argc != 2)
    {
        printf("ERROR USAGE\r\b");
        return -1;
    }
    filename = argv[1];
    fd = open(filename, O_RDWR | O_NONBLOCK);
    if (fd < 0)
    {
        printf("Can't open file:%s\r\n", filename);
        return -1;
    }
    
//使用poll
#if 0
    fds.fd = fd;
    fds.events = POLLIN;

    while (1)
    {
        ret = poll(&fds, 1, 500);
        if (ret)
        {
            ret = read(fd, &data, sizeof(data));
            if (ret < 0)
            {
                /* 读取出错 */
            }else{
                if (data)
                    printf("key value = %d \r\n", data);
            }
        }else if (ret == 0)
        {
            //超时处理
        }else if (ret < 0){
            //错误处理
        }
    }
#endif
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        ret = select(fd+1, &readfds, NULL, NULL, &timeout);
        switch (ret)
        {
        case 0:
            /* 超时处理 */
            break;
        case -1:
            //错误处理
            break;
        default:
            if (FD_ISSET(fd, &readfds))
            {
                ret = read(fd, &data, sizeof(data));
                if (ret < 0)
                {
                    /* code */
                }else{
                    printf("key value = %d \r\n",data);
                }
                
            }
            
            break;
        }
    }
    

    close(fd);
    return ret;
}