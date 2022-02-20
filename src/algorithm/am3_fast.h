#ifndef __AM3FAST_H__
#define __AM3FAST_H__

#include <cstdint>
#include <cstring>
#include <malloc.h>
#include <cassert>

using namespace std;

template <int N, int64_t P1>
class AM3FAST
{
public:
    const static int BITMAP_WIDTH = 8;
    const static int BITMAP_SIZE = N * BITMAP_WIDTH;

    int64_t __attribute__((aligned(64))) pow10[N + 8];
    int64_t __attribute__((aligned(64))) pow10inv[N + 8];
    int64_t __attribute__((aligned(64))) num[N]; // 不同长度的后缀

    uint8_t __attribute__((aligned(64))) bitmap[BITMAP_SIZE / BITMAP_WIDTH];

    int8_t __attribute__((aligned(64))) buffer[N];

    int k;
    int64_t b;

    int64_t pow(int64_t x, int64_t k)
    {
        if (k == 0)
            return 1;
        __int128_t ans = pow(x, k / 2);
        ans = (ans * ans) % P1;
        if (k & 1)
            ans = (ans * x) % P1;
        return ans;
    }

    AM3FAST()
    {
        memset(pow10, 0, sizeof(pow10));
        memset(num, 0, sizeof(num));

        pow10[0] = 1;
        for (int i = 1; i < N + 8; i++)
        {
            pow10[i] = (pow10[i - 1] * 10) % P1;
        }
        for (int i = 0; i < N + 8; i++)
        {
            pow10inv[i] = pow(pow10[i], P1 - 2);

            // __int128_t a = pow10[i];
            // __int128_t b = pow10inv[i];
            // __int128_t x = (a * b) % P1;
            // printf("inv i = %d, x == 1? %d\n", i, x == 1);
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
        for (int i = 0; i < N; i++)
        {
            buffer[i] = data[N - 1 - i] - '0';
        }

        memset(bitmap, 0, sizeof(bitmap));
        for (int i = 0; i < N; i++)
            if (buffer[i])
            {
                int pos = num[i] % BITMAP_SIZE;
                bitmap[pos / BITMAP_WIDTH] |= 1 << (pos % BITMAP_WIDTH);
            }

        k = 0;
        b = 0;
    }
    // 返回长度为多少的数字合法
    int append(int8_t ch)
    {
        k++;
        b = (b * 10 + ch) % P1;

        __int128_t nb = (b == 0) ? 0 : (P1 - b);
        int64_t need = (nb * pow10inv[k]) % P1;
        int pos = need % BITMAP_SIZE;

        if ((bitmap[pos / BITMAP_WIDTH] >> (pos % BITMAP_WIDTH)) & 1)
        {
            for (int i = 56; i < N; i++)
            {
                if (buffer[i] && (num[i] == need))
                {
                    return i + 1 + k;
                }
            }
        }

        return 0;
    }
};

#endif // __AM3FAST_H__
