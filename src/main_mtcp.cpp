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
#include <atomic>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <vector>
#include <set>
#include <mutex>
#include <queue>

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>

#include <mtcp_api.h>
#include <mtcp_epoll.h>

#include "common.h"
#include "algorithm/algorithm.h"
#include "algorithm/hashmap.h"
#include "algorithm/bitmap.h"

using namespace std;

// runtime assert, this assert cannot be disabled by compile flags
#define mtcp_assert(expr)                                                                                                                 \
    {                                                                                                                                     \
        bool passed = (expr);                                                                                                             \
        if (!passed)                                                                                                                      \
        {                                                                                                                                 \
            fprintf(stderr, "Error: mtcp_assert failed at %s:%d, expr = `%s`, errno = %s\n", __FILE__, __LINE__, #expr, strerror(errno)); \
            mtcp_exit();                                                                                                                  \
        }                                                                                                                                 \
    }

#define mtcp_assert_eq(a, b) mtcp_assert((a) == (b));

Algorithm<int32_t, MOST_N, HashMap<int32_t, 33554432>> alg_m1(MOST_M1_STR);
Algorithm<__int128_t, MOST_N, HashMap<__int128_t, 33554432>> alg_m2(MOST_M2_STR);
Algorithm<int64_t, MOST_N, HashMap<int64_t, 33554432>> alg_m3(MOST_M3_STR);
Algorithm<int64_t, MOST_N, HashMap<int64_t, 33554432>> alg_m4(MOST_M4_STR);

// Algorithm<int32_t, MOST_N, BitMap<int32_t, 1024>> alg_m1(MOST_M1_STR);
// Algorithm<__int128_t, MOST_N, BitMap<__int128_t, 1024>> alg_m2(MOST_M2_STR);
// Algorithm<int64_t, MOST_N, BitMap<int64_t, 1024>> alg_m3(MOST_M3_STR);
// Algorithm<int64_t, MOST_N, BitMap<int64_t, 1024>> alg_m4(MOST_M4_STR);

// 不同长度的http头
char __attribute__((aligned(64))) headers[128][1024 * 10];
int __attribute__((aligned(64))) headers_n[128];

char __attribute__((aligned(64))) buffer[1024 * 1024 * 256];
atomic<int> finished_cnt;
atomic<int> buffer_size;
atomic<double> received_time;

mctx_t mctx = 0;
vector<int> all_sockets;

queue<int> possible_ports;
std::mutex possible_ports_mtx;

void mtcp_exit()
{
    if (mctx)
    {
        for (int s : all_sockets)
        {
            mtcp_close(mctx, s);
        }
        sleep(1);
    }
    exit(-1);
}

void SignalHandler(int signum)
{
    mtcp_exit();
}

int connect_input()
{
    int socket_fd = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
    printf("input socket_fd = %d\n", socket_fd);
    all_sockets.push_back(socket_fd);

    int port;
    {
        std::lock_guard<std::mutex> lock(possible_ports_mtx);
        port = possible_ports.front();
        possible_ports.pop();
    }
    {
        struct sockaddr_in addr;

        bzero(&addr, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        mtcp_assert(inet_pton(AF_INET, "192.168.1.108", &addr.sin_addr) == 1);
        addr.sin_port = htons(port);

        mtcp_assert(mtcp_bind(mctx, socket_fd, (const sockaddr *)&addr, sizeof(addr)) == 0);
    }

    mtcp_setsock_nonblock(mctx, socket_fd);

    // connect
    {
        struct sockaddr_storage addr;
        memset(&addr, 0, sizeof(addr));

        struct sockaddr_in *addr_v4 = (struct sockaddr_in *)&addr;
        addr_v4->sin_family = AF_INET;
        addr_v4->sin_port = htons(10001);

        mtcp_assert(inet_pton(AF_INET, SERVER_ADDR, &addr_v4->sin_addr) == 1);
        mtcp_connect(mctx, socket_fd, (struct sockaddr *)addr_v4, sizeof(*addr_v4));

        printf("connect success\n");
    }

    // skip http header
    {
        char tmp[1024];
        while (true)
        {
            memset(tmp, 0, sizeof(tmp));
            errno = 0;
            ssize_t n = mtcp_read(mctx, socket_fd, tmp, sizeof(tmp));
            if (n < 0 && errno == EAGAIN)
                continue;
            if (strstr(tmp, "\r\n\r\n"))
                break;
        }
        printf("skip http header success\n");
    }

    return socket_fd;
}

int connect_post()
{
    int socket_fd = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
    mtcp_assert(socket_fd >= 0);
    all_sockets.push_back(socket_fd);

    int port;
    {
        std::lock_guard<std::mutex> lock(possible_ports_mtx);
        port = possible_ports.front();
        possible_ports.pop();
    }
    {
        struct sockaddr_in addr;

        bzero(&addr, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        mtcp_assert(inet_pton(AF_INET, "192.168.1.108", &addr.sin_addr) == 1);
        addr.sin_port = htons(port);

        mtcp_assert(mtcp_bind(mctx, socket_fd, (const sockaddr *)&addr, sizeof(addr)) == 0);
    }

    // no delay
    {
        int flag = 1;
        int result = mtcp_setsockopt(mctx, socket_fd, /* socket affected */
                                     IPPROTO_TCP,     /* set option at TCP level */
                                     TCP_NODELAY,     /* name of option */
                                     (char *)&flag,   /* the cast is historical cruft */
                                     sizeof(int));    /* length of option value */
        mtcp_assert_eq(result, 0);
    }

    // connect
    {
        struct sockaddr_storage addr;
        memset(&addr, 0, sizeof(addr));

        struct sockaddr_in *addr_v4 = (struct sockaddr_in *)&addr;
        addr_v4->sin_family = AF_INET;
        addr_v4->sin_port = htons(10002);

        mtcp_assert_eq(inet_pton(AF_INET, SERVER_ADDR, &addr_v4->sin_addr), 1);

        mtcp_assert_eq(mtcp_connect(mctx, socket_fd, (struct sockaddr *)addr_v4, sizeof(*addr_v4)), 0);
    }

    mtcp_setsock_nonblock(mctx, socket_fd);

    headers_n[socket_fd] = sprintf(headers[socket_fd], "POST /submit HTTP/1.1\r\n");
    return socket_fd;
}

void submit(int submit_fd, int begin, int end)
{
    int len = end - begin;
    int size = headers_n[submit_fd];
    size += sprintf(headers[submit_fd] + size, "Content-Length: %d\r\n\r\n", len);
    memcpy(headers[submit_fd] + size, buffer + begin, len);
    int n = mtcp_write(mctx, submit_fd, headers[submit_fd], size + len);
    mtcp_assert_eq(n, size + len);
}

template <typename ALG>
void run_alg(int cpu, const char *flag, ALG &alg)
{
    bind_cpu(cpu);

    int submit_fd = connect_post();
    while (buffer_size.load() < MOST_N)
        ;

    double __attribute__((aligned(64))) send_times[1024];
    int __attribute__((aligned(64))) send_begin[1024];
    int __attribute__((aligned(64))) send_end[1024];
    int send_times_N;

    int L = buffer_size.load();
    while (true)
    {
        alg.init(L, buffer);
        while (buffer_size.load() == L)
            ;
        double rtime = received_time.load();
        int R = buffer_size.load();
        send_times_N = 0;

        for (int k = L; k < R; k++)
        {
            if (buffer[k] != '0')
            {
                alg.update_prefix(k, (k - L) + MOST_N, buffer[k]);
            }
            int begin = alg.find();
            if (begin > 0)
            {
                submit(submit_fd, begin, k + 1);

                send_times[send_times_N] = get_timestamp();
                send_begin[send_times_N] = begin;
                send_end[send_times_N] = k + 1;
                send_times_N++;
            }
        }
        L = R;

        finished_cnt.fetch_add(1);
        while (finished_cnt.load() < 4)
            ;

        for (int i = 0; i < send_times_N; i++)
        {
            printf("submit %s %.6lf %s\n", flag, send_times[i] - rtime, std::string(buffer + send_begin[i], send_end[i] - send_begin[i]).c_str());
        }
        {
            char buf[1024];
            while (true)
            {
                int n = mtcp_read(mctx, submit_fd, buf, sizeof(buf));
                mtcp_assert(n > 0 || errno == EAGAIN);
                if (n <= 0)
                    break;
            }
        }
    }
}

int main()
{
    // Seed RNG
    srand(time(NULL));

    {
        set<int> ports;
        for (int i = 0; i < 1000; i++)
        {
            int port;
            while (true)
            {
                port = rand() % 10000 + 1025;
                if (ports.find(port) == ports.end())
                    break;
            }
            ports.insert(port);
            possible_ports.push(port);
        }
    }

    struct mtcp_conf mcfg;

    mtcp_getconf(&mcfg);
    mcfg.num_cores = 1;
    mtcp_setconf(&mcfg);

    if (mtcp_init("dpdk_client.conf"))
    {
        mtcp_assert(false);
        return -1;
    }

    mtcp_getconf(&mcfg);
    mcfg.max_concurrency = 32;
    mcfg.max_num_buffers = 32;
    mtcp_setconf(&mcfg);

    mtcp_register_signal(SIGINT, SignalHandler);

    mctx = mtcp_create_context(0);
    {
        struct sockaddr_in daddr;
        daddr.sin_family = AF_INET;
        daddr.sin_addr.s_addr = inet_addr("192.168.1.80");
        daddr.sin_port = htons(2047);
        mtcp_init_rss(mctx, INADDR_ANY, 1, daddr.sin_addr.s_addr, daddr.sin_port);
    }

    int __attribute__((aligned(64))) input_sockets[N_INPUT_SOCKET];
    int __attribute__((aligned(64))) input_sockets_pos[N_INPUT_SOCKET];
    for (int i = 0; i < N_INPUT_SOCKET; i++)
    {
        input_sockets[i] = connect_input();
        input_sockets_pos[i] = 0;
    }
    buffer_size.store(0);

    thread tm2([&]()
               { run_alg(2, "M2", alg_m2); });
    thread tm3([&]()
               { run_alg(8, "M3", alg_m3); });
    thread tm4([&]()
               { run_alg(9, "M4", alg_m4); });

    bind_cpu(1);
    int submit_fd = connect_post();

    { // sync sockets
        double last_recv_time = -1;
        while (true)
        {
            char buf[1024];
            for (int i = 0; i < N_INPUT_SOCKET; i++)
            {
                int n = mtcp_read(mctx, input_sockets[i], buf, sizeof(buf));
                mtcp_assert(n > 0 || errno == EAGAIN);
                if (n > 0)
                {
                    last_recv_time = get_timestamp();
                }
            }

            if (last_recv_time > 0 && last_recv_time + 0.7 < get_timestamp())
            {
                break;
            }
        }
        printf("sync sockets done!\n");
    }

    while (buffer_size.load() < MOST_N)
    {
        for (int i = 0; i < N_INPUT_SOCKET; i++)
        {
            int n = mtcp_read(mctx, input_sockets[i], buffer + input_sockets_pos[i], 1024);
            mtcp_assert(n > 0 || errno == EAGAIN);

            if (n > 0)
            {
                input_sockets_pos[i] += n;
                buffer_size.store(max(buffer_size.load(), input_sockets_pos[i]));
            }
        }
    }

    alg_m1.init(buffer_size.load(), buffer);
    double __attribute__((aligned(64))) send_times[1024];
    int __attribute__((aligned(64))) send_begin[1024];
    int __attribute__((aligned(64))) send_end[1024];
    int send_times_N;
    while (true)
    {
        for (int i = 0; i < N_INPUT_SOCKET; i++)
        {
            received_time.store(get_timestamp());
            int n = mtcp_read(mctx, input_sockets[i], buffer + input_sockets_pos[i], 1024);
            mtcp_assert(n > 0 || errno == EAGAIN);

            if (n > 0)
            {
                send_times_N = 0;
                input_sockets_pos[i] += n;

                if (input_sockets_pos[i] > buffer_size.load())
                {
                    int L = buffer_size.load();
                    int R = input_sockets_pos[i];
                    finished_cnt.store(0);
                    buffer_size.store(R);

                    for (int k = L; k < R; k++)
                    {
                        if (buffer[k] != '0')
                        {
                            alg_m1.update_prefix(k, (k - L) + MOST_N, buffer[k]);
                        }
                        if ((buffer[k] - '0') % 2 == 0)
                        {
                            int begin = alg_m1.find();
                            if (begin > 0)
                            {
                                submit(submit_fd, begin, k + 1);

                                send_times[send_times_N] = get_timestamp();
                                send_begin[send_times_N] = begin;
                                send_end[send_times_N] = k + 1;
                                send_times_N++;
                            }
                        }
                    }

                    alg_m1.init(buffer_size.load(), buffer);
                    finished_cnt.fetch_add(1);
                    while (finished_cnt.load() < 4)
                        ;
                }

                for (int i = 0; i < send_times_N; i++)
                {
                    printf("submit M1 %.6lf %s\n", send_times[i] - received_time.load(), std::string(buffer + send_begin[i], send_end[i] - send_begin[i]).c_str());
                }
                {
                    char buf[1024];
                    while (true)
                    {
                        int n = mtcp_read(mctx, submit_fd, buf, sizeof(buf));
                        mtcp_assert(n > 0 || errno == EAGAIN);
                        if (n <= 0)
                            break;
                    }
                }

                fflush(stdout);
            }
        }
    }

    return 0;
}
