#ifndef __AM3_H__
#define __AM3_H__

#include <cstdint>
#include <cstring>
#include <malloc.h>
#include <cassert>

using namespace std;

template <int N, int64_t P1>
class AM3
{
public:
    int64_t __attribute__((aligned(64))) pow10[N];
    int64_t __attribute__((aligned(64))) pow10buf10[10][N];
    int64_t __attribute__((aligned(64))) num[N];
    int8_t *buffers[2];
    int buffer_pos;

    AM3()
    {
        memset(pow10, 0, sizeof(pow10));
        memset(num, 0, sizeof(num));
        buffers[0] = (int8_t *)memalign(64, sizeof(*buffers[0]) * N);
        buffers[1] = (int8_t *)memalign(64, sizeof(*buffers[0]) * N);
        buffer_pos = 0;

        pow10[0] = 1;
        for (int i = 1; i < N; i++)
        {
            pow10[i] = (pow10[i - 1] * 10) % P1;
        }
        for (int x = 0; x < 10; x++)
        {
            for (int i = 0; i < N; i++)
            {
                pow10buf10[x][i] = ((x * pow10[i] % P1) * 10) % P1;
            }
        }
    }
    // data: 长度为N的初始数据
    void init(int8_t *data)
    {
        // 初始num
        for (int i = 0; i < N; i++)
        {
            num[i] = 0;
            for (int j = N - 1 - i; j < N; j++)
            {
                num[i] = (num[i] * 10 + (data[j] - '0')) % P1;
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
        __builtin_assume_aligned(num, 64);
        __builtin_assume_aligned(pow10, 64);
        for (int i = 0; i < N; i++)
        {
            num[i] = (num[i] * 10 - pow10buf10[buf[i]][i]) % P1;
        }
    }
    // 返回长度为多少的数字合法
    int append1(int8_t ch)
    {
        {
            __builtin_assume_aligned(num, 64);
#pragma vector aligned
            for (int i = 56; i < N; i++)
            {
                num[i] = num[i] + ch;
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
            __builtin_assume_aligned(num, 64);
#pragma vector aligned
            for (int i = 56; i < N; i++)
            {
                // if (num[i] == 0 || num[i] == P1)
                // {
                //     if (buf[i] != 0)
                //     {
                //         return i + 1;
                //     }
                // }
                ans += (i + 1) * int(num[i] == 0 || num[i] == P1) * int(buf[i] != 0);
            }
        }

        return ans;
    }

    void append2()
    {
        { // 每个数去掉各自的第一个
            auto *buf = buffers[buffer_pos];
            __builtin_assume_aligned(buf, 64);
            __builtin_assume_aligned(num, 64);
            __builtin_assume_aligned(pow10, 64);

            const int CHUNK_SIZE = 8;
            static_assert(N % CHUNK_SIZE == 0);

            for (int pos = 56; pos < N; pos += CHUNK_SIZE)
            {
                int64_t __attribute__((aligned(64))) __pow10buf10[CHUNK_SIZE];
                for (int i = 0; i < CHUNK_SIZE; i++)
                {
                    __pow10buf10[i] = pow10buf10[buf[i + pos]][i + pos];
                }

                auto *num_ptr = num + pos;
                __builtin_assume_aligned(num, 64);
                for (int i = 0; i < CHUNK_SIZE; i++)
                {
                    num_ptr[i] = num_ptr[i] * 10 - __pow10buf10[i];
                }
                for (int i = 0; i < CHUNK_SIZE; i++)
                {
                    num_ptr[i] %= P1;
                }
            }
        }
    }
};

#endif // __AM3_H__
