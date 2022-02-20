#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <cassert>
#include "algorithm/am1_fast_2.h"

#include "rt_assert.h"

using namespace std;

const int BUFFER_SIZE = 1 * 1024 * 1024; // 1MB

int main()
{
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(0, &mask);
        rt_assert_eq(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask), 0);
    }

    uint8_t *data = (uint8_t *)memalign(64, sizeof(uint8_t) * BUFFER_SIZE);
    {
        FILE *file = fopen("../data.bin", "rb");
        rt_assert_eq(fread(data, 1, BUFFER_SIZE, file), BUFFER_SIZE);
        fclose(file);

        for (int i = 0; i < BUFFER_SIZE; i++)
        {
            rt_assert('0' <= data[i] && data[i] <= '9');
        }
    }

    {
        AM1FAST<256, 20079859, 512> a;
        a.init((int8_t *)data);

        int cnt = 0;
        int next_size = 50; // 每次50 ~ 300之间
        for (int i = 256; i + next_size <= BUFFER_SIZE;)
        {
            int k = 0;
            int found_end;
            int found_len;

            while (a.append((int8_t *)data + i, next_size, k, found_end, found_len))
            {
                cnt++;
            }

            i += next_size;
            a.init((int8_t *)data + (i - 256));
            if (next_size == 300)
            {
                next_size = 50;
            }
            else
            {
                next_size++;
            }
        }
        cout << cnt << endl; // 527
    }

    return 0;
}
