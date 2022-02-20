#ifndef __AM1FAST_H__
#define __AM1FAST_H__

#include <cstdint>
#include <cstring>
#include <malloc.h>
#include "gcd.h"
#include "../rt_assert.h"

using namespace std;

template <int N, int32_t M, int MAX_APPEND>
class AM1FAST
{
public:
    const static int BITMAP_WIDTH = 8;
    const static int BITMAP_SIZE = N * BITMAP_WIDTH * 8;

    int32_t __attribute__((aligned(64))) pow10[N + MAX_APPEND];
    int32_t __attribute__((aligned(64))) pow10inv[N + MAX_APPEND];
    int32_t __attribute__((aligned(64))) num[N]; // 不同长度的后缀

    uint8_t __attribute__((aligned(64))) bitmap[BITMAP_SIZE / BITMAP_WIDTH];
    uint8_t __attribute__((aligned(64))) bitmap2[BITMAP_SIZE / BITMAP_WIDTH];

    int32_t __attribute__((aligned(64))) appends[MAX_APPEND];

    int8_t __attribute__((aligned(64))) buffer[N];

    AM1FAST()
    {
        memset(pow10, 0, sizeof(pow10));
        memset(num, 0, sizeof(num));

        pow10[0] = 1;
        for (int i = 1; i < N + MAX_APPEND; i++)
        {
            pow10[i] = (pow10[i - 1] * 10) % M;
        }
        for (int i = 0; i < N + MAX_APPEND; i++)
        {
            pow10inv[i] = inv<int64_t>(pow10[i], M);
            rt_assert(pow10inv[i] >= 0);

            int64_t a = pow10[i];
            int64_t b = pow10inv[i];
            int64_t x = (a * b) % M;
            rt_assert(x == 1);
            // printf("inv i = %d, x == 1? %d\n", i, x == 1);
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
        memset(bitmap2, 0, sizeof(bitmap2));
    }
    // 将需要的数据载入cache中
    void prepare()
    {
        int64_t volatile sum = 0;
        for (int i = 0; i < N; i++)
        {
            sum += buffer[i];
            sum += num[i];
        }
        sum = 0;
        for (int i = 0; i < BITMAP_SIZE / BITMAP_WIDTH; i++)
        {
            sum += bitmap[i];
            sum += bitmap2[i];
        }
        sum = 0;
        for (int i = min(MAX_APPEND - 1, 100); i >= 0; i--)
        {
            sum += appends[i];
        }
        sum = 0;
        for (int i = min(N + MAX_APPEND - 1, 100); i >= 0; i--)
        {
            sum += pow10inv[i];
        }
        sum = 0;
    }
    // 返回是否找到
    bool append(int8_t *data, int data_size, int &found_end, int &found_len)
    {
        int32_t need1 = 0; // 和前面拼起来
        int32_t need2 = 0; // 新数据独立产生

        for (int k = 0; k < data_size; k++)
        {
            int8_t ch = data[k] - '0';

            need1 = (need1 - ch * pow10inv[k + 1]) % M;
            if (need1 < 0)
                need1 += M;

            {
                int pos = need1 % BITMAP_SIZE;
                if (ch == 0 && ((bitmap[pos / BITMAP_WIDTH] >> (pos % BITMAP_WIDTH)) & 1))
                {
                    for (int i = 56; i < N; i++)
                    {
                        if (buffer[i] && (num[i] == need1))
                        {
                            found_end = k;
                            found_len = k + 1 + i + 1;
                            return true;
                        }
                    }
                }
            }

            {
                need2 = (need2 + ch * pow10inv[k + 1]) % M;
                int pos = need2 % BITMAP_SIZE;
                if (ch == 0 && ((bitmap2[pos / BITMAP_WIDTH] >> (pos % BITMAP_WIDTH)) & 1))
                {
                    for (int i = 0; i < k; i++)
                    {
                        if (data[i + 1] != '0' && appends[i] == need2)
                        {
                            found_end = k;
                            found_len = k - i;
                            return true;
                        }
                    }
                }
                appends[k] = need2;
                bitmap2[pos / BITMAP_WIDTH] |= 1 << (pos % BITMAP_WIDTH);
            }
        }

        return false;
    }
};

#endif // __AM1FAST_H__
