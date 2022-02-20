#ifndef __AM4_FAST_H__
#define __AM4_FAST_H__

#include <cstdint>
#include <cstring>
#include <malloc.h>
#include <cassert>

using namespace std;

template <int N, int POW3, int POW7, int POW11>
class AM4FAST
{
public:
    int64_t P3;
    int64_t P7;
    int64_t P11;

    int64_t __attribute__((aligned(64))) pow10_3[N];
    int64_t __attribute__((aligned(64))) pow10_7[N];
    int64_t __attribute__((aligned(64))) pow10_11[N];
    int64_t __attribute__((aligned(64))) num_3[N];
    int64_t __attribute__((aligned(64))) num_7[N];
    int64_t __attribute__((aligned(64))) num_11[N];
    int8_t *buffers[2];
    int buffer_pos;

    AM4FAST()
    {
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
        P11 = 1;
        for (int i = 0; i < POW11; i++)
        {
            P11 *= 11;
        }

        memset(pow10_3, 0, sizeof(pow10_3));
        memset(pow10_7, 0, sizeof(pow10_7));
        memset(pow10_11, 0, sizeof(pow10_11));
        memset(num_3, 0, sizeof(num_3));
        memset(num_7, 0, sizeof(num_7));
        memset(num_11, 0, sizeof(num_11));
        buffers[0] = (int8_t *)memalign(64, sizeof(*buffers[0]) * N);
        buffers[1] = (int8_t *)memalign(64, sizeof(*buffers[0]) * N);
        buffer_pos = 0;

        pow10_3[0] = 1;
        pow10_7[0] = 1;
        pow10_11[0] = 1;
        for (int i = 1; i < N; i++)
        {
            pow10_3[i] = (pow10_3[i - 1] * 10) % P3;
            pow10_7[i] = (pow10_7[i - 1] * 10) % P7;
            pow10_11[i] = (pow10_11[i - 1] * 10) % P11;
        }
    }
    // data: 长度为N的初始数据
    void init(int8_t *data)
    {
        // 初始num
        for (int i = 0; i < N; i++)
        {
            num_3[i] = 0;
            num_7[i] = 0;
            num_11[i] = 0;
            for (int j = N - 1 - i; j < N; j++)
            {
                num_3[i] = (num_3[i] * 10 + data[j] - '0') % P3;
                num_7[i] = (num_7[i] * 10 + data[j] - '0') % P7;
                num_11[i] = (num_11[i] * 10 + data[j] - '0') % P11;
            }
        }

        // 初始buffer
        auto *buf = buffers[buffer_pos];
        __builtin_assume_aligned(buf, 64);
        for (int i = 0; i < N; i++)
        {
            buf[i] = data[N - 1 - i] - '0';
        }

        // 每个数去掉各自的第一个
#pragma vector aligned
        for (int i = 0; i < N; i++)
        {
            num_3[i] = (num_3[i] - buf[i] * pow10_3[i]) * 10 % P3;
            num_7[i] = (num_7[i] - buf[i] * pow10_7[i]) * 10 % P7;
            num_11[i] = (num_11[i] - buf[i] * pow10_11[i]) * 10 % P11;
        }
    }
    // 返回长度为多少的数字合法
    int append1(int8_t ch)
    {
        {
            __builtin_assume_aligned(num_3, 64);
            __builtin_assume_aligned(num_7, 64);
            __builtin_assume_aligned(num_11, 64);
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
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                num_11[i] = num_11[i] + ch;
            }
        }

        { // 设置buffer
            buffer_pos ^= 1;
            memcpy(buffers[buffer_pos] + 1, buffers[buffer_pos ^ 1], sizeof(buffers[0][0]) * (N - 1));
            buffers[buffer_pos][0] = ch;
        }

        // int ans = 0;
        {
            auto *buf = buffers[buffer_pos];
            __builtin_assume_aligned(buf, 64);
            __builtin_assume_aligned(num_3, 64);
            __builtin_assume_aligned(num_7, 64);
            __builtin_assume_aligned(num_11, 64);
#pragma vector aligned
            for (int i = 0; i < N; i++)
            {
                if (num_11[i] == 0 || num_11[i] == P11)
                {
                    if (num_7[i] == 0 || num_7[i] == P7)
                    {
                        if (num_3[i] == 0 || num_3[i] == P3)
                        {
                            if (buf[i] != 0)
                            {
                                return i + 1;
                            }
                        }
                    }
                }
            }
        }

        return 0;
    }

    void append2()
    {
        { // 每个数去掉各自的第一个
            auto *buf = buffers[buffer_pos];
            __builtin_assume_aligned(buf, 64);
            __builtin_assume_aligned(num_3, 64);
            __builtin_assume_aligned(num_7, 64);
            __builtin_assume_aligned(num_11, 64);
            __builtin_assume_aligned(pow10_3, 64);
            __builtin_assume_aligned(pow10_7, 64);
            __builtin_assume_aligned(pow10_11, 64);

            const int CHUNK_SIZE = 8;
            static_assert(N % CHUNK_SIZE == 0);

            for (int pos = 0; pos < N; pos += CHUNK_SIZE)
            {
                for (int i = pos; i < pos + CHUNK_SIZE; i++)
                {
                    num_3[i] = (num_3[i] - buf[i] * pow10_3[i]) * 10 % P3;
                }
                for (int i = pos; i < pos + CHUNK_SIZE; i++)
                {
                    num_7[i] = (num_7[i] - buf[i] * pow10_7[i]) * 10 % P7;
                }
                for (int i = pos; i < pos + CHUNK_SIZE; i++)
                {
                    num_11[i] = (num_11[i] - buf[i] * pow10_11[i]) * 10 % P11;
                }
            }
        }
    }
};

#endif // __AM4_FAST_H__
