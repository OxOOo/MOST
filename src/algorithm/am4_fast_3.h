#ifndef __AM4FAST_H__
#define __AM4FAST_H__

#include <cstdint>
#include <cstring>
#include <malloc.h>

using namespace std;

template <int N, int32_t M>
class AM4
{
public:
    int32_t __attribute__((aligned(64))) pow10[N];
    int32_t __attribute__((aligned(64))) pow10_buf[10][N];
    int32_t __attribute__((aligned(64))) num[N];
    int8_t *buffers[2]; // 存0-9
    int buffer_pos;

    AM4()
    {
        memset(pow10, 0, sizeof(pow10));
        memset(num, 0, sizeof(num));
        buffers[0] = (int8_t *)memalign(64, sizeof(*buffers[0]) * N);
        buffers[1] = (int8_t *)memalign(64, sizeof(*buffers[0]) * N);
        buffer_pos = 0;

        pow10[0] = 1;
        for (int i = 1; i < N; i++)
        {
            pow10[i] = (pow10[i - 1] * 10) % M;
        }
        for (int i = 0; i < N; i++)
        {
            for (int x = 0; x < 10; x++)
            {
                pow10_buf[x][i] = (pow10[i] * x) % M;
            }
        }
    }
    // data: 长度为N的初始数据
    void init(int8_t *data) // 原始数据
    {
        // 初始num
        for (int i = 0; i < N; i++)
        {
            num[i] = 0;
            for (int j = N - 1 - i; j < N; j++)
            {
                num[i] = (num[i] * 10 + (data[j] - '0')) % M;
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
#pragma vector aligned
        for (int i = 0; i < N; i++)
        {
            num[i] = (num[i] - buf[i] * pow10[i]) * 10 % M;
        }
    }
    void init_buffer(int8_t *data)
    {
        auto *buf = buffers[buffer_pos];
        __builtin_assume_aligned(buf, 64);
        for (int i = 0; i < N; i++)
        {
            buf[i] = data[N - 1 - i] - '0';
        }
    }
    // 返回长度为多少的数字合法
    template <int BEGIN, int END>
    int append1(int8_t ch) // 0-9
    {
        {
            __builtin_assume_aligned(num, 64);
#pragma vector aligned
            for (int i = BEGIN; i < END; i++)
            {
                num[i] = num[i] + ch;
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
            __builtin_assume_aligned(num, 64);
#pragma vector aligned
            for (int i = BEGIN; i < END; i++)
            {
                // ans += (i + 1) * int(num[i] == 0 || num[i] == M) * int(buf[i] != 0);
                if (num[i] == 0 || num[i] == M)
                {
                    if (buf[i] != 0)
                    {
                        return i + 1;
                    }
                }
            }
        }

        return 0;
    }

    template <int BEGIN, int END>
    void append2()
    {
        { // 每个数去掉各自的第一个
            auto *buf = buffers[buffer_pos];
            __builtin_assume_aligned(num, 64);
            __builtin_assume_aligned(pow10, 64);
#pragma vector aligned
            for (int i = BEGIN; i < END; i++)
            {
                num[i] = (num[i] - buf[i] * pow10[i]) * 10 % M;
                // num[i] = (num[i] - pow10_buf[buf[i]][i]) * 10 % M;
            }
        }
    }
};

#endif // __AM4FAST_H__
