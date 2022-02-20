#ifndef __AM4_128_H__
#define __AM4_128_H__

#include <cstdint>
#include <cstring>
#include <malloc.h>
#include <cassert>

using namespace std;

template <int N, int POW2, int POW3, int POW7>
class AM4_128
{
public:
    __int128_t P2;
    int64_t P3;
    int64_t P7;

    __int128_t __attribute__((aligned(64))) pow10_2[N];
    int64_t __attribute__((aligned(64))) pow10_3[N];
    int64_t __attribute__((aligned(64))) pow10_7[N];
    __int128_t __attribute__((aligned(64))) num_2[N];
    int64_t __attribute__((aligned(64))) num_3[N];
    int64_t __attribute__((aligned(64))) num_7[N];
    int8_t *buffers[2];
    int buffer_pos;

    AM4_128()
    {
        P2 = __int128_t(1) << POW2;
        P3 = 1;
        for (int i = 0; i < POW3; i++)
        {
            P3 *= 3;
        }
        P7 = 1;
        for (int i = 0; i < POW7; i++)
        {
            P7 *= 7;
        }

        memset(pow10_2, 0, sizeof(pow10_2));
        memset(pow10_3, 0, sizeof(pow10_3));
        memset(pow10_7, 0, sizeof(pow10_7));
        memset(num_2, 0, sizeof(num_2));
        memset(num_3, 0, sizeof(num_3));
        memset(num_7, 0, sizeof(num_7));
        buffers[0] = (int8_t *)memalign(64, sizeof(*buffers[0]) * N);
        buffers[1] = (int8_t *)memalign(64, sizeof(*buffers[0]) * N);
        buffer_pos = 0;

        pow10_2[0] = 1;
        pow10_3[0] = 1;
        pow10_7[0] = 1;
        for (int i = 1; i < N; i++)
        {
            pow10_2[i] = (pow10_2[i - 1] * 10) % P2;
            pow10_3[i] = (pow10_3[i - 1] * 10) % P3;
            pow10_7[i] = (pow10_7[i - 1] * 10) % P7;
        }
    }
    // data: 长度为N的初始数据
    void init(int8_t *data)
    {
        // 初始num
        for (int i = 0; i < N; i++)
        {
            num_2[i] = 0;
            num_3[i] = 0;
            num_7[i] = 0;
            for (int j = N - 1 - i; j < N; j++)
            {
                num_2[i] = (num_2[i] * 10 + data[j]) % P2;
                num_3[i] = (num_3[i] * 10 + data[j]) % P3;
                num_7[i] = (num_7[i] * 10 + data[j]) % P7;
            }
        }

        // 初始buffer
        auto *buf = buffers[buffer_pos];
        __builtin_assume_aligned(buf, 64);
        for (int i = 0; i < N; i++)
        {
            buf[i] = data[N - 1 - i];
        }

        // 每个数去掉各自的第一个
#pragma vector aligned
        for (int i = 0; i < N; i++)
        {
            num_2[i] = (num_2[i] - buf[i] * pow10_2[i]) * 10 % P2;
            num_3[i] = (num_3[i] - buf[i] * pow10_3[i]) * 10 % P3;
            num_7[i] = (num_7[i] - buf[i] * pow10_7[i]) * 10 % P7;
        }
    }
    // 返回长度为多少的数字合法
    int append1(int8_t ch)
    {
        {
            __builtin_assume_aligned(num_2, 64);
            __builtin_assume_aligned(num_3, 64);
            __builtin_assume_aligned(num_7, 64);
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_2[i] = num_2[i] + ch;
            }
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_3[i] = num_3[i] + ch;
            }
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_7[i] = num_7[i] + ch;
            }
        }

        { // 设置buffer
            buffer_pos ^= 1;
            memcpy(buffers[buffer_pos] + 1, buffers[buffer_pos ^ 1], sizeof(buffers[0][0]) * (N - 1));
            buffers[buffer_pos][0] = ch;
        }

        int ans = 0;
        {
            auto *buf = buffers[buffer_pos];
            __builtin_assume_aligned(buf, 64);
            __builtin_assume_aligned(num_2, 64);
            __builtin_assume_aligned(num_3, 64);
            __builtin_assume_aligned(num_7, 64);
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                ans += (i + 1) * int(num_2[i] == 0 || num_2[i] == P2) * int(num_3[i] == 0 || num_3[i] == P3) * int(num_7[i] == 0 || num_7[i] == P7) * int(buf[i] != 0);
            }
        }

        return ans;
    }

    void append2()
    {
        { // 每个数去掉各自的第一个
            auto *buf = buffers[buffer_pos];
            __builtin_assume_aligned(buf, 64);
            __builtin_assume_aligned(num_2, 64);
            __builtin_assume_aligned(num_3, 64);
            __builtin_assume_aligned(num_7, 64);
            __builtin_assume_aligned(pow10_2, 64);
            __builtin_assume_aligned(pow10_3, 64);
            __builtin_assume_aligned(pow10_7, 64);
            // for (int i = 0; i < N; i++)
            // {
            //     num[i] = ((num[i] - buf[i] * pow10[i]) * 10) % M;
            // }

#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_2[i] = num_2[i] - buf[i] * pow10_2[i];
            }
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_2[i] = num_2[i] * 10 % P2;
            }
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_3[i] = num_3[i] - buf[i] * pow10_3[i];
            }
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_3[i] = num_3[i] * 10 % P3;
            }
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_7[i] = num_7[i] - buf[i] * pow10_7[i];
            }
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_7[i] = num_7[i] * 10 % P7;
            }
        }
    }
};

#endif // __AM4_128_H__
