#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define FIFO_NAME_PIPE "/tmp/testp"

int main()
{
    if (access(FIFO_NAME_PIPE, F_OK) == 0)
    {
        //printf("%s already exist\n", FIFO_NAME_PIPE);
        remove(FIFO_NAME_PIPE);
    }
    int rtn = mkfifo(FIFO_NAME_PIPE, 0777);
    if (rtn != 0)
    {
        printf("mkfifo error, code=%d\n", errno);
        return -1;
    }
    int pipe_fd = open(FIFO_NAME_PIPE, O_RDWR);
    if (pipe_fd != -1)
    {
        int flags = 0;
        if (flags = fcntl(pipe_fd, F_GETFL, 0) < 0)
        {
            printf("fcntl getfl error, code=%d\n", errno);
            close(pipe_fd);
            return -1;
        }
        flags |= O_NONBLOCK;
        if (fcntl(pipe_fd, F_SETFL, flags) < 0)
        {
            printf("fcntl setfl error, code=%d\n", errno);
            close(pipe_fd);
            return -1;
        }

        const char* content = "hello fifo_pipe";
        int ret = write(pipe_fd, content, strlen(content));
        if (ret == -1)
        {
            printf("write pipe error, code=%d\n", errno);
        }
        else
        {
            printf("write pipe success, byte=%d\n", ret);
        }
        // 不休眠，close会把管道内容清空
        sleep(30);
        close(pipe_fd);
    }
    else
    {
        printf("open pipe error, code=%d\n", errno);
    }
    return 0;
}