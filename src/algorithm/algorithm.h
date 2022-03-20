#ifndef __ALGORITHM_H__
#define __ALGORITHM_H__

#include "gcd.h"
#include "../rt_assert.h"

#define MAXPOW 1024

template<typename T>
T fast_multiply(T a, T b, T M)
{
    if (b == 0) return 0;
    T ans = fast_multiply<T>(a, b/2, M);
    ans = (ans + ans) % M;
    if (b & 1)
    {
        ans = (ans + a) % M;
    }
    return ans;
}

template<typename T, int N, typename MAP_T>
class Algorithm
{
    T __attribute__((aligned(64))) M, prefix;
    T __attribute__((aligned(64))) pow10[MAXPOW];
    T __attribute__((aligned(64))) inv10[MAXPOW];

    MAP_T map;

public:
    Algorithm(const char* Ms)
    {
        M = 0;
        for (int i = 0; Ms[i]; i ++)
        {
            M = M * 10 + Ms[i] - '0';
        }

        pow10[0] = 1;
        for (int i = 1; i < MAXPOW; i ++)
        {
            pow10[i] = (pow10[i-1] * 10) % M;
        }
        for (int i = 0; i < MAXPOW; i ++)
        {
            inv10[i] = inv<T>(pow10[i], M);

            rt_assert(fast_multiply<T>(pow10[i], inv10[i], M) == 1);
        }
    }

    inline void init(int end, const char* data)
    {
        map.init();

        prefix = 0;
        for (int i = N; i >= 1; i --)
        {
            if (data[end - i] == '0') continue;
            map.set(prefix, end-i);
            prefix = (prefix + (data[end - i] - '0') * inv10[N-i]) % M;
        }
    }

    inline void update_prefix(int pos, int len, char ch)
    {
        map.set(prefix, pos);
        prefix = (prefix + (ch - '0') * inv10[len]) % M;
    }

    inline int find()
    {
        return map.find(prefix);
    }
};

#endif // __ALGORITHM_H__
