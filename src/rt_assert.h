#ifndef __RT_ASSERT_H__
#define __RT_ASSERT_H__

#include <cstdio>
#include <cstdlib>

#include <sstream>

#define __abort()                                                       \
    {                                                                   \
        fprintf(stderr, "aborted at file %s:%d\n", __FILE__, __LINE__); \
        abort();                                                        \
    }

// runtime assert, this assert cannot be disabled by compile flags
#define rt_assert(expr)                                                                                    \
    {                                                                                                      \
        bool passed = (expr);                                                                              \
        if (!passed)                                                                                       \
        {                                                                                                  \
            fprintf(stderr, "Error: rt_assert failed at %s:%d, expr = `%s`\n", __FILE__, __LINE__, #expr); \
            fprintf(stderr, "errno = %s", strerror(errno));                                                \
            exit(-1);                                                                                      \
        }                                                                                                  \
    }

#define rt_assert_eq(expr1, expr2)                                                                                \
    {                                                                                                             \
        const auto v1 = (expr1);                                                                                  \
        const auto v2 = (expr2);                                                                                  \
        if (v1 != v2)                                                                                             \
        {                                                                                                         \
            std::stringstream msg;                                                                                \
            msg << "`" << #expr1 << "` (got " << v1 << ") != `" << #expr2 << "` (got " << v2 << ")";              \
            fprintf(stderr, "Error: rt_assert_eq failed at %s:%d, %s.\n", __FILE__, __LINE__, msg.str().c_str()); \
            fprintf(stderr, "errno = %s", strerror(errno));                                                       \
            exit(-1);                                                                                             \
        }                                                                                                         \
    }

#define rt_assert_ne(expr1, expr2)                                                                                \
    {                                                                                                             \
        const auto v1 = (expr1);                                                                                  \
        const auto v2 = (expr2);                                                                                  \
        if (v1 == v2)                                                                                             \
        {                                                                                                         \
            std::stringstream msg;                                                                                \
            msg << "`" << #expr1 << "` (got " << v1 << ") == `" << #expr2 << "` (got " << v2 << ")";              \
            fprintf(stderr, "Error: rt_assert_ne failed at %s:%d, %s.\n", __FILE__, __LINE__, msg.str().c_str()); \
            fprintf(stderr, "errno = %s", strerror(errno));                                                       \
            exit(-1);                                                                                             \
        }                                                                                                         \
    }

#endif // __RT_ASSERT_H__
