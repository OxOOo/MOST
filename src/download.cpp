#include <iostream>
#include <cstdint>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <atomic>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <fcntl.h>
#include <sys/select.h>

#include "rt_assert.h"

using namespace std;

// 获取当前时间，单位：秒
inline double get_timestamp()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main()
{
    char path[1024];
    sprintf(path, "data/download/%.0lf.txt", get_timestamp());
    FILE *fout = fopen(path, "wb");
    rt_assert(fout);

    int recv_fd = -1;
    {
        recv_fd = socket(AF_INET, SOCK_STREAM, 0);
        printf("recv_fd = %d\n", recv_fd);

        struct sockaddr_storage addr;
        memset(&addr, 0, sizeof(addr));

        struct sockaddr_in *addr_v4 = (struct sockaddr_in *)&addr;
        addr_v4->sin_family = AF_INET;
        addr_v4->sin_port = htons(10001);

        rt_assert_eq(inet_pton(AF_INET, "172.1.1.119", &addr_v4->sin_addr), 1);

        rt_assert_eq(connect(recv_fd, (struct sockaddr *)addr_v4, sizeof(*addr_v4)), 0);

        printf("connect success\n");
    }

    for (int i = 0; i < 10; i++)
    {
        char tmp[1024];
        read(recv_fd, tmp, sizeof(tmp));
    }

    while (true)
    {
        char tmp[1024];
        int n = read(recv_fd, tmp, sizeof(tmp));
        rt_assert(n > 0);

        rt_assert_eq(fwrite(tmp, 1, n, fout), n);
        fflush(fout);

        printf("read\n");
        fflush(stdout);
    }

    return 0;
}
