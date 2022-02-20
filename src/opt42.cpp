#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <cassert>
#include <cstring>
#include <vector>
#include <algorithm>
#include "algorithm/am4_fast_5.h"

#include "rt_assert.h"

using namespace std;

const int BUFFER_SIZE = 1 * 1024 * 1024; // 1MB

// 获取当前时间，单位：秒
inline double get_timestamp()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

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
        // AM4<256, 19487171> a;
        // AM4<256, 531441> a;
        AM4FAST<256, 19487171, 512> a;
        a.init((int8_t *)data);

        vector<double> timespans;

        int cnt = 0;
        int next_size = 300; // 每次300 ~ 500之间
        for (int i = 256; i + next_size <= BUFFER_SIZE;)
        {
            double received_time = get_timestamp();

            int k = 0;
            int found_end;
            int found_len;

            while (a.append((int8_t *)data + i, next_size, k, found_end, found_len))
            {
                cnt++;
                timespans.push_back(get_timestamp() - received_time);
            }

            i += next_size;
            a.init((int8_t *)data + (i - 256));

            if (next_size == 500)
            {
                next_size = 300;
            }
            else
            {
                next_size++;
            }
        }
        cout << cnt << endl; // 519

        sort(timespans.begin(), timespans.end());
        cout << "min: " << timespans[0] << endl;
        cout << "max: " << timespans[timespans.size() - 1] << endl;
        double sum = 0;
        for (double item : timespans)
        {
            sum += item;
        }
        cout << "mean: " << sum / timespans.size() << endl;
    }

    // {
    //     char *plain = new char[BUFFER_SIZE];
    //     for (int i = 0; i < BUFFER_SIZE; i++)
    //     {
    //         plain[i] = data[i] + '0';
    //     }

    //     AM4FAST<512, 56, 35, 20> a;
    //     a.init((int8_t *)data);

    //     for (int i = 512; i < BUFFER_SIZE; i++)
    //     {
    //         if (i % 1024 == 0)
    //         {
    //             a.init((int8_t *)data + (i - 512));
    //         }

    //         int k = a.append1(data[i]);
    //         if (k >= 1)
    //         {
    //             if (1 <= k && k <= 512)
    //             {
    //                 printf("%s\n", string(plain + (i - k + 1), k).c_str());
    //             }
    //             else
    //             {
    //                 printf("=========== %d\n", k);
    //             }
    //         }
    //         a.append2();
    //     }
    // }

    return 0;
}
