#include"hash.h"
#include"common.h"

typedef struct hash_node // hash节点结构体类型
{
	void *key; // 关键值指针（用于管理保存关键值的空间）
	void *value; // 实值指针（用于管理保存实值的空间）
	struct hash_node *prev; // 前驱节点指针
	struct hash_node *next; // 后驱节点指针
} hash_node_t;

struct hash // hash结构体类型
{
	unsigned int buckets; // hash桶的大小
	hashfunc_t hash_func; // hash函数映射方式
	hash_node_t **nodes; // hash桶的二级指针
};

hash_node_t** hash_get_bucket(hash_t *hash, void *key); // 获取关键值为（*key）的节点所在的链表首地址
hash_node_t* hash_get_node_by_key(hash_t *hash, void *key, unsigned int key_size); // 根据关键值查询节点

hash_t *hash_alloc(unsigned int buckets, hashfunc_t hash_func) // 创建哈希桶
{
	hash_t *hash_pri = (hash_t *)malloc(sizeof(hash_t)); // 为hash桶开辟空间
	assert(hash_pri != NULL);
	hash_pri->buckets = buckets; // 保存桶的大小
	hash_pri->hash_func = hash_func; // 保存映射函数
	int size = buckets * sizeof(hash_node_t *); // 计算桶直接管理的空间大小（单位：字节）
	hash_pri->nodes = (hash_node_t **)malloc(size); // 开辟管理空间
	memset(hash_pri->nodes, 0, size); // 初始化空间
	return hash_pri;
}

void* hash_lookup_entry(hash_t *hash, void* key, unsigned int key_size) // 查询关键值为key的节点的实值
{
	hash_node_t *node = hash_get_node_by_key(hash, key, key_size); // 根据关键值查询节点 
	if (node == NULL) // 如果没有查到节点
	{
		return NULL;
	} 
	return node->value; // 返回查找到的节点的实值
}

void hash_add_entry(hash_t *hash, void *key, unsigned int key_size,void *value, unsigned int value_size)  // 增加hash节点
{
	if (hash_lookup_entry(hash, key, key_size)) // 首先查找hash中是否关键值为key的节点，如果有这个节点存在，提示该节点重复
	{
		fprintf(stderr, "duplicate hash key\n"); // 提示节点重复
		return;
	} 
	// 如果当前节点不存在则重新申请一个节点
	hash_node_t *node = malloc(sizeof(hash_node_t)); // 申请一个hash节点空间
	node->prev = NULL; 
	node->next = NULL;
	node->key = malloc(key_size); // 为关键值开辟空间
	memcpy(node->key, key, key_size); // 将关键值填充到空间中
	node->value = malloc(value_size); // 开辟实值空间
	memcpy(node->value, value, value_size); // 将实值填充到空间中
	
	hash_node_t **bucket = hash_get_bucket(hash, key); // 获取关键值为（*key）的节点所在的链表首地址
	if (*bucket == NULL) // 如果当前链表是空链表，则说明当前类型新建节点是第一个节点
	{
		*bucket = node; // 此时*bucket是指向第一个节点的指针，
	} 
	else // 如果不是空链表，则头插
	{
		// 将新节点插入到链表头部
		node->next = *bucket; // 新节点的后驱指针指向头节点
		(*bucket)->prev = node; // 头节点的前驱指针指向新节点
		*bucket = node; // 新节点称为头节点
	}
}

void hash_free_entry(hash_t *hash, void *key, unsigned int key_size)
{
	hash_node_t *node = hash_get_node_by_key(hash, key, key_size);
	if (node == NULL)
	{
		return;
	} 
	free(node->key);
	free(node->value);
	if (node->prev)
	{
		node->prev->next = node->next;
	}
	else
	{
		hash_node_t **bucket = hash_get_bucket(hash, key); // 获取关键值为（*key）的节点所在的链表首地址
		*bucket = node->next;
	} 
	if(node->next)
		node->next->prev = node->prev;
	free(node);
}

hash_node_t** hash_get_bucket(hash_t *hash, void *key) // 获取关键值为（*key）的节点所在的链表首地址（简单来说，就是找到key所在的链表）
{
	unsigned int bucket = hash->hash_func(hash->buckets, key); // 调用hash映射函数，查找关键值为（*key）的节点在哪条链表中
	if (bucket >= hash->buckets) // 如果目标节点不在hash桶中，则报错
	{
		fprintf(stderr, "bad bucket lookup\n");
		exit(EXIT_FAILURE);
	}
	return &(hash->nodes[bucket]); // 返回关键值为（*key）的节点所在的链表首地址
}

hash_node_t* hash_get_node_by_key(hash_t *hash, void *key, unsigned int key_size) // 根据关键值查询节点
{
	hash_node_t **bucket = hash_get_bucket(hash, key); // 获取关键值为（*key）的节点所在的链表首地址
	hash_node_t *node = *bucket; // node为指向链表第一个节点的指针
	if (node == NULL) // 如果当前链表为空，则说明查询节点不存在，直接返回NULL
	{
		return NULL;
	}
	while (node != NULL && memcmp(node->key, key, key_size) != 0) // 循环查找当前链表中关键值为（*key）的节点
	{
		node = node->next;
	}
	return node; // 返回查找到的节点
}