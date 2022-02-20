#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <cassert>
#include <vector>
#include <algorithm>
#include "algorithm/am3.h"

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
        AM3<256, 500000000000000147> a;
        a.init((int8_t *)data);

        vector<double> timespans;

        int cnt = 0;
        int next_size = 50; // 每次50 ~ 300之间
        for (int i = 256; i + next_size <= BUFFER_SIZE;)
        {
            double received_time = get_timestamp();

            for (int k = 0; k < next_size; k++)
            {
                if (a.append1(data[i + k] - '0') >= 1)
                {
                    cnt++;
                    timespans.push_back(get_timestamp() - received_time);
                }
                a.append2();
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

    return 0;
}
