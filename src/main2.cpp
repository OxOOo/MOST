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
#include "algorithm/a128_predict.h"

using namespace std;

#define BIND_CORE 1
#define N 512

#define USER "lammbda"
#define PASSWORD "kkfmdcgU"

const int BUFFER_SIZE = 1 * 1024 * 1024 * 1024; // 1GB
uint8_t *buffer = new uint8_t[BUFFER_SIZE];
int buffer_size = 0;

A128PREDICT<N, 20> am2("104648257118348370704723099");

// 提交所使用的TCP连接
int submit_fd;

// 不同长度的http头
char headers[N * 2 + 10][1024 * 10];
int headers_n[N * 2 + 10];

// 获取当前时间，单位：秒
inline double get_timestamp()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// 生成提交所使用的TCP连接池
void gen_submit_fd()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    rt_assert(socket_fd >= 0);

    // no delay
    {
        int flag = 1;
        int result = setsockopt(socket_fd,     /* socket affected */
                                IPPROTO_TCP,   /* set option at TCP level */
                                TCP_NODELAY,   /* name of option */
                                (char *)&flag, /* the cast is historical cruft */
                                sizeof(int));  /* length of option value */
        rt_assert_eq(result, 0);
    }

    // connect
    {
        struct sockaddr_storage addr;
        memset(&addr, 0, sizeof(addr));

        struct sockaddr_in *addr_v4 = (struct sockaddr_in *)&addr;
        addr_v4->sin_family = AF_INET;
        addr_v4->sin_port = htons(10002);

        rt_assert_eq(inet_pton(AF_INET, "47.95.111.217", &addr_v4->sin_addr), 1);

        rt_assert_eq(connect(socket_fd, (struct sockaddr *)addr_v4, sizeof(*addr_v4)), 0);
    }

    submit_fd = socket_fd;
    printf("generate one submit fd = %d\n", submit_fd);
}

double received_time;

int sent_cnt;
struct sent_t
{
    double sent_time;
    uint8_t *begin;
    int len;
    int type;
} sends[1024];

inline void submit(int type, uint8_t *begin, int len)
{
    memcpy(headers[len] + headers_n[len], begin, len);

    int ret = write(submit_fd, headers[len], headers_n[len] + len);
    if (ret != headers_n[len] + len)
    {
        fprintf(stderr, "===== submit_fd = %d %d %s\n", submit_fd, errno, strerror(errno));
        exit(-1);
    }

    sends[sent_cnt] = {
        .sent_time = get_timestamp(),
        .begin = begin,
        .len = len,
        .type = type};
    sent_cnt++;
}

int main()
{
    rt_assert(BIND_CORE >= 0);
    if (BIND_CORE >= 0)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(BIND_CORE, &mask);
        rt_assert_eq(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask), 0);
    }

    memset(buffer, 0, BUFFER_SIZE);

    gen_submit_fd();

    for (int i = 1; i <= N * 2; i++)
    {
        headers_n[i] = sprintf(headers[i], "POST /submit?user=%s&passwd=%s HTTP/1.1\r\nContent-Length: %d\r\n\r\n", USER, PASSWORD, i);
    }

    const int RECV_FD_NUM = 5;
    int recv_fds[1024];
    int sync_pos[1024];
    int max_nfds = 0;
    memset(sync_pos, 0, sizeof(sync_pos));

    for (int i = 0; i < RECV_FD_NUM; i++)
    {
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        printf("socket_fd = %d\n", socket_fd);

        struct sockaddr_storage addr;
        memset(&addr, 0, sizeof(addr));

        struct sockaddr_in *addr_v4 = (struct sockaddr_in *)&addr;
        addr_v4->sin_family = AF_INET;
        addr_v4->sin_port = htons(10001);

        rt_assert_eq(inet_pton(AF_INET, "47.95.111.217", &addr_v4->sin_addr), 1);

        rt_assert_eq(connect(socket_fd, (struct sockaddr *)addr_v4, sizeof(*addr_v4)), 0);

        printf("connect success\n");

        // 非阻塞
        rt_assert_eq(fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL) | O_NONBLOCK), 0);

        recv_fds[i] = socket_fd;
        max_nfds = max(max_nfds, socket_fd + 1);
    }

    // 第一步，跳过5秒钟
    {
        double start_time = get_timestamp();
        while (true)
        {
            fd_set rd;
            FD_ZERO(&rd);
            for (int i = 0; i < RECV_FD_NUM; i++)
            {
                FD_SET(recv_fds[i], &rd);
            }
            select(max_nfds, &rd, NULL, NULL, NULL);

            if (start_time + 5 < get_timestamp())
                break;

            for (int i = 0; i < RECV_FD_NUM; i++)
            {
                int socket_fd = recv_fds[i];
                if (FD_ISSET(socket_fd, &rd))
                {
                    char tmp[1024];
                    read(socket_fd, tmp, 1024);
                }
            }
        }
        printf("skip 5s success\n");
    }

    // 第二步：如果连续0.5秒没有收到数据，则说明数据流已经同步了
    {
        while (true)
        {
            fd_set rd;
            FD_ZERO(&rd);
            for (int i = 0; i < RECV_FD_NUM; i++)
            {
                FD_SET(recv_fds[i], &rd);
            }
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 500 * 1000;
            int err = select(max_nfds, &rd, NULL, NULL, &tv);
            if (err == 0)
                break;

            for (int i = 0; i < RECV_FD_NUM; i++)
            {
                int socket_fd = recv_fds[i];
                if (FD_ISSET(socket_fd, &rd))
                {
                    char tmp[1024];
                    read(socket_fd, tmp, 1024);
                }
            }
        }
        printf("sync success\n");
    }

    // 第三步：等待前N个数据
    {
        while (true)
        {
            fd_set rd;
            FD_ZERO(&rd);
            for (int i = 0; i < RECV_FD_NUM; i++)
            {
                FD_SET(recv_fds[i], &rd);
            }
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 500 * 1000;
            int err = select(max_nfds, &rd, NULL, NULL, &tv);
            if (err == 0)
            {
                if (buffer_size >= N)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            rt_assert(err > 0);

            for (int i = 0; i < RECV_FD_NUM; i++)
            {
                int socket_fd = recv_fds[i];
                if (FD_ISSET(socket_fd, &rd))
                {
                    size_t n = read(socket_fd, buffer + sync_pos[i], 1024);
                    rt_assert(n > 0);

                    if (buffer_size > sync_pos[i])
                    {
                        // pass
                    }
                    else
                    {
                        buffer_size += n;
                    }
                    sync_pos[i] += n;
                }
            }
        }
        printf("recv first N data success\n");
    }

    am2.init((int8_t *)buffer + (buffer_size - N));
    int min_possible_size = 0;

    while (true)
    {
        fd_set rd;
        FD_ZERO(&rd);
        for (int i = 0; i < RECV_FD_NUM; i++)
        {
            FD_SET(recv_fds[i], &rd);
        }

        double begin_read_time = get_timestamp();
        int err = select(max_nfds, &rd, NULL, NULL, NULL);

        if (err == 0) //超时
        {
            rt_assert(false);
        }
        else if (err == -1) //失败
        {
            rt_assert(false);
        }

        for (int i = 0; i < RECV_FD_NUM; i++)
        {
            int socket_fd = recv_fds[i];
            if (FD_ISSET(socket_fd, &rd))
            {
                size_t n = read(socket_fd, buffer + sync_pos[i], 1024);
                received_time = get_timestamp();
                sent_cnt = 0;

                // printf("fd = %d, %lf, len = %d\n", socket_fd, get_timestamp(), n);
                rt_assert(n > 0);

                if (buffer_size > sync_pos[i])
                {
                    // pass
                }
                else
                {
                    for (int k = sync_pos[i]; k < sync_pos[i] + n; k++)
                    {
                        int len = am2.append1(buffer[k] - '0');
                        if (k >= min_possible_size && 1 <= len && len <= N)
                        {
                            submit(2, buffer + (k - len + 1), len);

                            int llen = len;
                            for (int kk = k + 1; buffer[kk] == '0'; kk++)
                            {
                                llen++;
                                submit(-2, buffer + (k - len + 1), llen);
                                min_possible_size = kk + 1;
                            }
                        }

                        if (k + 1 < sync_pos[i] + n)
                        {
                            am2.append2();
                        }
                    }

                    buffer_size += n;

                    int exist_len;
                    int predict_len;
                    if (am2.predict(exist_len, predict_len))
                    {
                        memcpy(buffer + buffer_size, am2.predicts, predict_len);
                        submit(102, buffer + (buffer_size - exist_len), exist_len + predict_len);
                        min_possible_size = buffer_size + predict_len;
                    }

                    am2.append2();

                    for (int i = 0; i < sent_cnt; i++)
                    {
                        printf("%d ", sends[i].type);
                        printf("%.6lf ", sends[i].sent_time - received_time);
                        char tmp[N + 1024];
                        memset(tmp, 0, sizeof(tmp));
                        memcpy(tmp, sends[i].begin, sends[i].len);
                        printf("%s", tmp);
                        printf("\n");
                    }
                    // printf("next_submit_fd_pos = %d, read time = %.6lf\n", next_submit_fd_pos, received_time - begin_read_time);
                    printf("read time = %.6lf\n", received_time - begin_read_time);
                    fflush(stdout);
                }
                sync_pos[i] += n;
            }
        }
    }

    return 0;
}
