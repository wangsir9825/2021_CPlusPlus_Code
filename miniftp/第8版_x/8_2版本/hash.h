#ifndef _HASH_H_
#define _HASH_H_


typedef struct hash hash_t; // 声明一个hash结构

typedef unsigned int (*hashfunc_t)(unsigned int, void*); // hash函数指针，参数包括：桶大小，数据指针

hash_t* hash_alloc(unsigned int buckets, hashfunc_t hash_func); // 申请hash表，参数包括：桶的大小，hash函数，返回hash地址

void* hash_lookup_entry(hash_t *hash, void* key, unsigned int key_size); // 根据关键值查询节点

void hash_add_entry(hash_t *hash, void *key, unsigned int key_size,void *value, unsigned int value_size); // 增加hash节点

void hash_free_entry(hash_t *hash, void *key, unsigned int key_size); // 释放hash节点


#endif /* _HASH_H_ */