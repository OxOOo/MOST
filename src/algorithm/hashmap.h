#ifndef __HASHMAP_H__
#define __HASHMAP_H__

// 4194304

template<typename KEY_T, int HASHMAP_SIZE>
class HashMap
{
    int hash_idx[HASHMAP_SIZE];
public:

    inline void init()
    {
        memset(hash_idx, 0, sizeof(hash_idx));
    }

    inline void set(const KEY_T key, int value)
    {
        hash_idx[key % HASHMAP_SIZE] = value;
    }

    inline int find(const KEY_T key)
    {
        return hash_idx[key % HASHMAP_SIZE];
    }
};

#endif // __HASHMAP_H__
