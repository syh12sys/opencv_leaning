#include <cstdio>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

int proc_is_exist(const char* procname)
{
    char filePath[256] = { 0 };
    sprintf(filePath, "/var/run/%s.pid", procname);
    int fd = open(filePath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        printf("open %s error, code=%d \n", filePath, errno);
        return 1;
    }

    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    if (fcntl(fd, F_SETLK, &fl) < 0)
    {
        printf("lock %s error\n", filePath);
        return 1;
    }
    else
    {
        ftruncate(fd, 0);
        char buf[128] = { 0 };
        sprintf(buf, "%d", getpid());
        write(fd, buf, strlen(buf));
        return 0;
    }
}

int main()
{
    int exist = proc_is_exist("ProcessSingle");
    if (exist == 0)
    {
        printf("ProcessSingle not exist\n");
    }
    else
    {
        printf("ProcessSingle already exist\n");
    }
    int exitState = 0;
    int pid = waitpid(31099, &exitState, 0);
    if (pid < 0)
    {
        printf("waitpid error, code=%d\n", errno);
    }
    return 0;
}