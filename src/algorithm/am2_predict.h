#ifndef __A128_PREDICT_H__
#define __A128_PREDICT_H__

#include <cstdint>
#include <cstring>
#include <malloc.h>
#include <cassert>

using namespace std;

template <int N, int MAX_PREDICT>
class A128PREDICT
{
public:
    __int128_t M;
    __int128_t __attribute__((aligned(64))) pow10[N];
    __int128_t __attribute__((aligned(64))) pow10buf[10][N];
    __int128_t __attribute__((aligned(64))) num[N];
    int8_t *buffers[2];
    int buffer_pos;

    uint8_t __attribute__((aligned(64))) predicts[1024];
    static_assert(MAX_PREDICT < 1024);

    A128PREDICT(const char *Ms)
    {
        M = 0;
        for (int i = 0; Ms[i]; i++)
        {
            M = M * 10 + Ms[i] - '0';
        }

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
        for (int x = 0; x < 10; x++)
        {
            for (int i = 0; i < N; i++)
            {
                pow10buf[x][i] = (x * pow10[i]) % M;
            }
        }
    }
    // data: 长度为N的初始数据
    void init(int8_t *data)
    {
        // 初始num
        for (int i = 32; i < N; i++)
        {
            num[i] = 0;
            for (int j = N - 1 - i; j < N; j++)
            {
                num[i] = (num[i] * 10 + data[j] - '0') % M;
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
        for (int i = 32; i < N; i++)
        {
            num[i] = (num[i] - buf[i] * pow10[i]) * 10 % M;
        }
    }
    // 返回长度为多少的数字合法
    int append1(int8_t ch)
    {
        {
            __builtin_assume_aligned(num, 64);
#pragma vector aligned
            for (int i = 32; i < N; i++)
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
            for (int i = 32; i < N; i++)
            {
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

    void append2()
    {
        { // 每个数去掉各自的第一个
            auto *buf = buffers[buffer_pos];
            __builtin_assume_aligned(buf, 64);
            __builtin_assume_aligned(num, 64);
            __builtin_assume_aligned(pow10, 64);
            for (int i = 32; i < N; i++)
            {
                num[i] = ((num[i] - pow10buf[buf[i]][i]) * 10) % M;
            }
        }
    }

    int num64length(uint64_t x)
    {
        int ans = 0;
        while (x > 0)
        {
            ans++;
            x /= 10;
        }
        return ans;
    }
    int num128length(__uint128_t x)
    {
        if ((x >> 64) == 0)
            return num64length(x);
        int ans = 0;
        while ((x >> 64) > 0)
        {
            ans++;
            x /= 10;
        }
        return ans + num64length(x);
    }

    __int128_t mul(__int128_t a, __int128_t b)
    {
        __int128_t res = 0;
        while (b > 0)
        {
            if (b & 1)
                res = (res + a) % M;
            a = (a + a) % M;
            b >>= 1;
        }
        return res;
    }

    // 如果预测到了，则返回true
    bool predict(int &exist_len, int &predict_len)
    {
        auto *buf = buffers[buffer_pos];
        __builtin_assume_aligned(buf, 64);

        for (int k = 1; k <= MAX_PREDICT; k++)
        {
            for (int i = 32; i + k < N; i++)
                if (buf[i] != 0)
                {
                    __int128_t b = -mul(num[i], pow10[k]) % M;
                    if (b < 0)
                    {
                        b += M;
                    }
                    int len = num128length(b);
                    if (len <= k)
                    {
                        for (int j = k - 1; j >= 0; j--)
                        {
                            predicts[j] = b % 10 + '0';
                            b /= 10;
                        }
                        exist_len = i + 1;
                        predict_len = k;

                        return true;
                    }
                }
        }
        return false;
    }
};

#endif // __A128_PREDICT_H__
