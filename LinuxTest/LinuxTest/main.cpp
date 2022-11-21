#include <cstdio>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <cerrno>

void sig_handle(int signo)
{
    printf("sig_handle: tid=%d, signo=%d\n", syscall(SYS_gettid), signo);
    //if (signal(SIGQUIT, SIG_DFL) == SIG_ERR)
    //{
    //    printf("无法为SIGQUIT信号设置缺省处理（终止进程）\n");
    //    exit(1);
    //}
}

void sig_ini_handle(int signo)
{
    printf("sig_ini_handle: tid=%d, signo=%d\n", syscall(SYS_gettid), signo);
}

int main()
{
    // test pip
    {
        int fds[2] = { 0 };
        if (pipe(fds) == -1)
        {
            printf("create pipe error");
        }
        else
        {
            pid_t pid = fork();
            if (pid == -1)
            {
                printf("fork error");
            }
            else
            {
                if (pid == 0)
                {
                    close(fds[0]);
                    char msg[] = "hello world";
                    int rnt = write(fds[1], msg, strlen(msg));
                    if (rnt != 0)
                    {
                        printf("child write error, code=%d\n", errno);
                    }
                    close(fds[1]);
                    exit(0);
                }
                else
                {
                    close(fds[1]);
                    char msg[128] = { 0 };
                    read(fds[0], msg, 128);
                    printf("message from child: %s\n", msg);
                    close(fds[0]);
                }
            }
        }
    }
    //if (signal(SIGINT, sig_ini_handle) == SIG_ERR)
    //{
    //    printf("无法捕获SIGINT信号\n");
    //    exit(1);
    //}
    //sleep(100);
    //printf("sleep被唤醒后，直接退出\n");
    //return 0;

    if (signal(SIGQUIT, sig_handle) == SIG_ERR)
    {
        printf("无法捕获SIGQUIT信号\n");
        exit(1);
    }

    sigset_t newmask, oldmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);
    //if (signal(SIGINT, sig_ini_handle) == SIG_ERR)
    //{
    //    printf("无法捕获SIGINT信号\n");
    //    exit(1);
    //}

    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
    {
        printf("sigprocmask error\n");
        exit(1);
    }

    //sleep(20);
    int dwSigNo;
    int dwRet = sigwait(&newmask, &dwSigNo);
    printf("sigwait returns %d, signo = %d, tid=%d\n", dwRet, dwSigNo, syscall(SYS_gettid));

    if (sigismember(&newmask, SIGINT))
    {
        printf("SIGINT响应成功, tid=%d\n", syscall(SYS_gettid));
    }
    else
    {
        printf("SIGINT响应失败\n");
    }

    if (sigismember(&newmask, SIGHUP))
    {
        printf("SIGHUP响应成功\n");
    }
    else
    {
        printf("SIGHUP响应失败\n");
    }

    return 0;
}