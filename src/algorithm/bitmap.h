#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <cstdint>

template<typename KEY_T, int BITMAP_SIZE>
class BitMap
{
    const int BITMAP_WIDTH = 8;
    uint8_t __attribute__((aligned(64))) bitmap[BITMAP_SIZE];
    KEY_T __attribute__((aligned(64))) keys[1024];
    int __attribute__((aligned(64))) values[1024];
    int size;
public:

    inline void init()
    {
        memset(bitmap, 0, sizeof(bitmap));
        size = 0;
    }

    inline void set(const KEY_T key, int value)
    {
        int pos = key % (BITMAP_SIZE * BITMAP_WIDTH);
        bitmap[pos / BITMAP_WIDTH] |= 1 << (pos % BITMAP_WIDTH);
        keys[size] = key;
        values[size] = value;
        size++;
    }

    inline int find(const KEY_T key)
    {
        int pos = key % (BITMAP_SIZE * BITMAP_WIDTH);

        if ((bitmap[pos / BITMAP_WIDTH] >> (pos % BITMAP_WIDTH)) & 1)
        {
            for (int i = 0; i < size; i ++)
            {
                if (keys[i] == key)
                {
                    return values[i];
                }
            }
        }
        return 0;
    }
};

#endif // __BITMAP_H__
