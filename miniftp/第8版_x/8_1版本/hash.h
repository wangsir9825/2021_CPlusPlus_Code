#ifndef _HASH_H_
#define _HASH_H_


typedef struct hash hash_t; // ����һ��hash�ṹ

typedef unsigned int (*hashfunc_t)(unsigned int, void*); // hash����ָ�룬����������Ͱ��С������ָ��

hash_t* hash_alloc(unsigned int buckets, hashfunc_t hash_func); // ����hash������������Ͱ�Ĵ�С��hash����������hash��ַ

void* hash_lookup_entry(hash_t *hash, void* key, unsigned int key_size); // ���ݹؼ�ֵ��ѯ�ڵ�

void hash_add_entry(hash_t *hash, void *key, unsigned int key_size,void *value, unsigned int value_size); // ����hash�ڵ�

void hash_free_entry(hash_t *hash, void *key, unsigned int key_size); // �ͷ�hash�ڵ�


#endif /* _HASH_H_ */