#include <cstdio>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <linux/cn_proc.h>
#include <linux/netlink.h>

#include <memory>
#include <functional>

int SetupNetlink()
{
    auto nl_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (nl_sock == -1)
    {
        printf("create socket error, code=%d", errno);
        return -1;
    }

    struct sockaddr_nl sa_nl;
    sa_nl.nl_family = AF_NETLINK;
    sa_nl.nl_groups = 1;
    sa_nl.nl_pid = (unsigned int)syscall(SYS_getpid);
    auto rc = bind(nl_sock, (struct sockaddr*)&sa_nl, sizeof(sa_nl));
    std::unique_ptr<int, std::function<void(int*)>> up(&nl_sock, [](int* fd) {close(*fd); });
    if (rc == -1)
    {
        printf("bind socket error, code=%d", errno);
        return -1;
    }
    return 0;
}

//int main()
//{
//    printf("%s 向你问好!\n", "ProcessCreateAndExit");
//    return 0;
//}