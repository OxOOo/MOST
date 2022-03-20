#ifndef __COMMON_H__
#define __COMMON_H__

#include "rt_assert.h"

#define MOST_N 256
#define MOST_M1_STR "59937251"
#define MOST_M2_STR "104648257118348370704723401"
#define MOST_M3_STR "500000000000000221"
#define MOST_M4_STR "79792266297612001"

#define SERVER_ADDR "192.168.1.80"

#define N_INPUT_SOCKET 8

static inline void bind_cpu(int core)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(core, &mask);
    rt_assert_eq(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask), 0);
}

// 获取当前时间，单位：秒
static inline double get_timestamp()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

#endif // __COMMON_H__
