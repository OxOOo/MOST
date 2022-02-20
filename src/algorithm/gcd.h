#ifndef __GCD_H__
#define __GCD_H__

#include "../rt_assert.h"

// solve a*x + b*y = g
template <class T>
void exgcd(T a, T b, T &g, T &x, T &y)
{
    if (b == 0)
    {
        g = a;
        x = 1;
        y = 0;
    }
    else
    {
        exgcd<T>(b, a % b, g, y, x);
        y -= a / b * x;
    }
}

// inv(x) * x % M = 1
template <class T>
T inv(T x, T M)
{
    T y, k, g;
    exgcd<T>(x, M, g, y, k);
    rt_assert(g == 1);
    return ((y % M) + M) % M;
}

#endif // __GCD_H__
