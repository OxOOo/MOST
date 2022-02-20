#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include "algorithm/am2_predict.h"

#include "rt_assert.h"

using namespace std;

const int BUFFER_SIZE = 1 * 1024 * 1024; // 1MB

int main()
{
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
        A128PREDICT<256, 21> a("104648257118348370704723119");
        a.init((int8_t *)data);

        int cnt = 0;
        int predict_cnt = 0;
        int predict_ac_cnt = 0;
        for (int i = 256; i < BUFFER_SIZE; i++)
        {
            if (i % 1024 == 0)
            {
                a.init((int8_t *)data + (i - 256));
            }

            cnt += a.append1(data[i] - '0') >= 1;

            if (i % 64 == 0)
            {
                int exist_len;
                int predict_len;
                if (a.predict(exist_len, predict_len))
                {
                    char tmp[1024];
                    memset(tmp, 0, sizeof(tmp));
                    memcpy(tmp, data + (i - exist_len + 1), exist_len);
                    memcpy(tmp + exist_len, a.predicts, predict_len);
                    printf("predict %d %d %s\n", exist_len, predict_len, tmp);

                    predict_cnt++;
                    if (memcmp(data + i + 1, a.predicts, predict_len) == 0)
                    {
                        predict_ac_cnt++;
                    }
                    printf("%d / %d\n", predict_ac_cnt, predict_cnt);
                }
            }

            a.append2();
        }
        cout << cnt << endl; // 512
    }

    return 0;
}
