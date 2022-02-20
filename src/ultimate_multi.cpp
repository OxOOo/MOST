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
#include "algorithm/am1_fast.h"
#include "algorithm/am2_fast.h"
#include "algorithm/am3_fast_2.h"
#include "algorithm/am4_fast_4.h"

using namespace std;

#define N 256
#define MAX_APPEND 512

#define USER "lammbda"
#define PASSWORD "kkfmdcgU"

// 是否使用TSC来度量时间
// 设为0表示使用clock_gettime系统调用来获取时间
// 设为1表示使用TSC寄存器来计算时间，不需要系统调用
#define USE_TSC_MEASURE_TIME 0

struct Info
{
    int orders_N;
    int orders[1024];
    int predict_m2;
};
Info info;

const int BUFFER_SIZE = 512 * 1024 * 1024; // 512MB
uint8_t *buffer = new uint8_t[BUFFER_SIZE];
int buffer_size = 0;

AM1FAST<N, 20079859, MAX_APPEND> am1_fast;
AM2FAST<N, MAX_APPEND> am2_fast("104648257118348370704723119");
AM3FAST<N, 500000000000000147, MAX_APPEND> am3_fast;
AM4FAST<N, 19487171, MAX_APPEND> am4_fast;

// 提交所使用的TCP
int submit_fd = -1;

// 不同长度的http头
char __attribute__((aligned(64))) headers[N * 2 + 10][1024 * 10];
int __attribute__((aligned(64))) headers_n[N * 2 + 10];

void load_info(const char *info_path)
{
    bool success = false;
    for (int i = 0; i < 10; i++)
    {
        FILE *file = fopen(info_path, "r");
        if (!file)
        {
            printf("load info failed\n");
            fflush(stdout);
            continue;
        }

        fscanf(file, "%d", &info.orders_N);
        for (int i = 0; i < info.orders_N; i++)
        {
            fscanf(file, "%d", &info.orders[i]);
        }
        fscanf(file, "%d", &info.predict_m2);
        fclose(file);

        printf("orders:");
        for (int i = 0; i < info.orders_N; i++)
        {
            printf(" %d", info.orders[i]);
        }
        printf(", predict_m2 = %d\n", info.predict_m2);
        fflush(stdout);
        success = true;
        break;
    }
    rt_assert(success);
}

#if USE_TSC_MEASURE_TIME
// Reference code : https://github.com/erpc-io/eRPC/blob/master/src/util/timer.h

// 读取TSC寄存器
static inline size_t rdtsc()
{
    uint64_t rax;
    uint64_t rdx;
    asm volatile("rdtsc"
                 : "=a"(rax), "=d"(rdx));
    return static_cast<size_t>((rdx << 32) | rax);
}

// 计算CPU频率，返回1ns内有多少个时钟周期
static double measure_rdtsc_freq()
{
    auto start_time = std::chrono::high_resolution_clock::now();
    const uint64_t rdtsc_start = rdtsc();

    // Do not change this loop! The hardcoded value below depends on this loop
    // and prevents it from being optimized out.
    uint64_t sum = 5;
    for (uint64_t i = 0; i < 1000000; i++)
    {
        sum += i + (sum + i) * (i % sum);
    }
    rt_assert(sum == 13580802877818827968ull);

    const uint64_t rdtsc_cycles = rdtsc() - rdtsc_start;

    auto end_time = std::chrono::high_resolution_clock::now();
    auto time_ns = static_cast<size_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time)
            .count());

    const double freq_ghz = rdtsc_cycles * 1.0 / time_ns;
    rt_assert(freq_ghz >= 0.5 && freq_ghz <= 5.0);

    return freq_ghz;
}

static double CPU_FREQ;
void init_time()
{
    CPU_FREQ = measure_rdtsc_freq();
    printf("CPU_FREQ = %.3lf\n", CPU_FREQ);
    fflush(stdout);
}

inline double get_timestamp()
{
    auto tsc = rdtsc();
    return (tsc) / CPU_FREQ * 1e-9;
}

#else

// 获取当前时间，单位：秒
inline double get_timestamp()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

#endif

// 生成提交所使用的TCP连接
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

        rt_assert_eq(inet_pton(AF_INET, "172.1.1.119", &addr_v4->sin_addr), 1);

        rt_assert_eq(connect(socket_fd, (struct sockaddr *)addr_v4, sizeof(*addr_v4)), 0);
    }

    submit_fd = socket_fd;
    printf("generate one submit fd, fd = %d\n", socket_fd);
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

char *current_multi_buf = NULL;
int current_multi_len;
inline void submit_multi_first(int type, uint8_t *begin, int len)
{
    current_multi_buf = headers[len];
    memcpy(current_multi_buf + headers_n[len], begin, len);
    current_multi_len = headers_n[len] + len;

    sends[sent_cnt] = {
        .sent_time = get_timestamp(),
        .begin = begin,
        .len = len,
        .type = type};
    sent_cnt++;
}
inline void submit_multi_append(int type, uint8_t *begin, int len)
{
    memcpy(current_multi_buf + current_multi_len, headers[len], headers_n[len]);
    memcpy(current_multi_buf + current_multi_len + headers_n[len], begin, len);
    current_multi_len += headers_n[len] + len;

    sends[sent_cnt] = {
        .sent_time = get_timestamp(),
        .begin = begin,
        .len = len,
        .type = type};
    sent_cnt++;
}
inline void submit_multi_end()
{
    int ret = write(submit_fd, current_multi_buf, current_multi_len);
    if (ret != current_multi_len)
    {
        fprintf(stderr, "###=====\n");
        exit(-1);
    }

    current_multi_buf = NULL;
    current_multi_len;
}

int main(int argc, char *argv[])
{
    rt_assert_eq(argc, 3);
    int bind_core_id = atoi(argv[1]);
    const char *info_path = argv[2];

    {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(bind_core_id, &mask);
        rt_assert_eq(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask), 0);
    }

#if USE_TSC_MEASURE_TIME
    init_time();
#endif

    memset(buffer, 0, BUFFER_SIZE);

    gen_submit_fd();

    for (int i = 1; i <= N * 2; i++)
    {
        headers_n[i] = sprintf(headers[i], "POST /submit?user=%s&passwd=%s HTTP/1.1\r\nContent-Length: %d\r\n\r\n", USER, PASSWORD, i);
    }

    const int RECV_FD_NUM = 3;
    int __attribute__((aligned(64))) recv_fds[1024];
    int __attribute__((aligned(64))) sync_pos[1024];
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

        rt_assert_eq(inet_pton(AF_INET, "172.1.1.119", &addr_v4->sin_addr), 1);

        rt_assert_eq(connect(socket_fd, (struct sockaddr *)addr_v4, sizeof(*addr_v4)), 0);

        printf("connect success\n");

        // 非阻塞
        rt_assert_eq(fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL) | O_NONBLOCK), 0);

        recv_fds[i] = socket_fd;
        max_nfds = max(max_nfds, socket_fd + 1);
    }

    // 第一步，跳过5s
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
                    int n = read(socket_fd, buffer + sync_pos[i], 1024);
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

    load_info(info_path);
    am1_fast.init((int8_t *)buffer + (buffer_size - N));
    am2_fast.init((int8_t *)buffer + (buffer_size - N));
    am3_fast.init((int8_t *)buffer + (buffer_size - N));
    am4_fast.init((int8_t *)buffer + (buffer_size - N));

    while (true)
    {
        fd_set rd;
        FD_ZERO(&rd);
        for (int i = 0; i < RECV_FD_NUM; i++)
        {
            FD_SET(recv_fds[i], &rd);
        }

        double begin_read_time = get_timestamp();
        for (int _o = 0; _o < info.orders_N; _o++)
        {
            int order = info.orders[_o];
            if (order == 1)
            {
                am1_fast.prepare();
            }
            if (order == 2)
            {
                am2_fast.prepare();
            }
            if (order == 3)
            {
                am3_fast.prepare();
            }
            if (order == 4)
            {
                am4_fast.prepare();
            }
        }
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
                int n = read(socket_fd, buffer + sync_pos[i], 1024);
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
                    for (int _o = 0; _o < info.orders_N; _o++)
                    {
                        int order = info.orders[_o];

                        if (order == 1)
                        {
                            int found_end;
                            int found_len;
                            if (am1_fast.append((int8_t *)buffer + buffer_size, min(n, MAX_APPEND), found_end, found_len))
                            {
                                found_end += buffer_size;

                                if (buffer[found_end + 1] == '0')
                                {
                                    submit_multi_first(1, buffer + (found_end - found_len + 1), found_len);

                                    int llen = found_len;
                                    for (int kk = found_end + 1; buffer[kk] == '0'; kk++)
                                    {
                                        llen++;
                                        submit_multi_append(-1, buffer + (found_end - found_len + 1), llen);
                                    }

                                    submit_multi_end();
                                }
                                else
                                {
                                    submit(1, buffer + (found_end - found_len + 1), found_len);
                                }
                            }
                        }

                        if (order == 2)
                        {
                            int found_end;
                            int found_len;
                            if (am2_fast.append((int8_t *)buffer + buffer_size, min(n, MAX_APPEND), found_end, found_len))
                            {
                                found_end += buffer_size;

                                if (buffer[found_end + 1] == '0')
                                {
                                    submit_multi_first(2, buffer + (found_end - found_len + 1), found_len);

                                    int llen = found_len;
                                    for (int kk = found_end + 1; buffer[kk] == '0'; kk++)
                                    {
                                        llen++;
                                        submit_multi_append(-2, buffer + (found_end - found_len + 1), llen);
                                    }

                                    submit_multi_end();
                                }
                                else
                                {
                                    submit(2, buffer + (found_end - found_len + 1), found_len);
                                }
                            }
                        }

                        if (order == 3)
                        {
                            int found_end;
                            int found_len;
                            if (am3_fast.append((int8_t *)buffer + buffer_size, min(n, MAX_APPEND), found_end, found_len))
                            {
                                found_end += buffer_size;

                                if (buffer[found_end + 1] == '0')
                                {
                                    submit_multi_first(3, buffer + (found_end - found_len + 1), found_len);

                                    int llen = found_len;
                                    for (int kk = found_end + 1; buffer[kk] == '0'; kk++)
                                    {
                                        llen++;
                                        submit_multi_append(-3, buffer + (found_end - found_len + 1), llen);
                                    }

                                    submit_multi_end();
                                }
                                else
                                {
                                    submit(3, buffer + (found_end - found_len + 1), found_len);
                                }
                            }
                        }

                        if (order == 4)
                        {
                            int found_end;
                            int found_len;
                            if (am4_fast.append((int8_t *)buffer + buffer_size, min(n, MAX_APPEND), found_end, found_len))
                            {
                                found_end += buffer_size;

                                if (buffer[found_end + 1] == '0')
                                {
                                    submit_multi_first(4, buffer + (found_end - found_len + 1), found_len);

                                    int llen = found_len;
                                    for (int kk = found_end + 1; buffer[kk] == '0'; kk++)
                                    {
                                        llen++;
                                        submit_multi_append(-4, buffer + (found_end - found_len + 1), llen);
                                    }

                                    submit_multi_end();
                                }
                                else
                                {
                                    submit(4, buffer + (found_end - found_len + 1), found_len);
                                }
                            }
                        }
                    }

                    buffer_size += n;

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
                    // printf("next_submit_fd_pos = %d\n", next_submit_fd_pos);
                    printf("read time = %.6lf\n", received_time - begin_read_time);
                    fflush(stdout);

                    load_info(info_path);
                    for (int _o = 0; _o < info.orders_N; _o++)
                    {
                        int order = info.orders[_o];

                        if (order == 1)
                        {
                            am1_fast.init((int8_t *)buffer + (buffer_size - N));
                        }
                        if (order == 2)
                        {
                            am2_fast.init((int8_t *)buffer + (buffer_size - N));
                        }
                        if (order == 3)
                        {
                            am3_fast.init((int8_t *)buffer + (buffer_size - N));
                        }
                        if (order == 4)
                        {
                            am4_fast.init((int8_t *)buffer + (buffer_size - N));
                        }
                    }
                }
                sync_pos[i] += n;
            }
        }

        { // 把submit返回的数据清空
            fd_set rd;
            FD_ZERO(&rd);
            FD_SET(submit_fd, &rd);
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1000;
            int err = select(submit_fd + 1, &rd, NULL, NULL, &tv);
            rt_assert(err >= 0);

            if (err > 0)
            {
                char tmp[1024];
                read(submit_fd, tmp, sizeof(tmp));
            }
        }
    }

    return 0;
}
