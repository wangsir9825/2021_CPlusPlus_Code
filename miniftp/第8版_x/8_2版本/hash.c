#include"hash.h"
#include"common.h"

typedef struct hash_node // hash�ڵ�ṹ������
{
	void *key; // �ؼ�ֵָ�루���ڹ�����ؼ�ֵ�Ŀռ䣩
	void *value; // ʵֵָ�루���ڹ�����ʵֵ�Ŀռ䣩
	struct hash_node *prev; // ǰ���ڵ�ָ��
	struct hash_node *next; // �����ڵ�ָ��
} hash_node_t;

struct hash // hash�ṹ������
{
	unsigned int buckets; // hashͰ�Ĵ�С
	hashfunc_t hash_func; // hash����ӳ�䷽ʽ
	hash_node_t **nodes; // hashͰ�Ķ���ָ��
};

hash_node_t** hash_get_bucket(hash_t *hash, void *key); // ��ȡ�ؼ�ֵΪ��*key���Ľڵ����ڵ������׵�ַ
hash_node_t* hash_get_node_by_key(hash_t *hash, void *key, unsigned int key_size); // ���ݹؼ�ֵ��ѯ�ڵ�

hash_t *hash_alloc(unsigned int buckets, hashfunc_t hash_func) // ������ϣͰ
{
	hash_t *hash_pri = (hash_t *)malloc(sizeof(hash_t)); // ΪhashͰ���ٿռ�
	assert(hash_pri != NULL);
	hash_pri->buckets = buckets; // ����Ͱ�Ĵ�С
	hash_pri->hash_func = hash_func; // ����ӳ�亯��
	int size = buckets * sizeof(hash_node_t *); // ����Ͱֱ�ӹ���Ŀռ��С����λ���ֽڣ�
	hash_pri->nodes = (hash_node_t **)malloc(size); // ���ٹ���ռ�
	memset(hash_pri->nodes, 0, size); // ��ʼ���ռ�
	return hash_pri;
}

void* hash_lookup_entry(hash_t *hash, void* key, unsigned int key_size) // ��ѯ�ؼ�ֵΪkey�Ľڵ��ʵֵ
{
	hash_node_t *node = hash_get_node_by_key(hash, key, key_size); // ���ݹؼ�ֵ��ѯ�ڵ� 
	if (node == NULL) // ���û�в鵽�ڵ�
	{
		return NULL;
	} 
	return node->value; // ���ز��ҵ��Ľڵ��ʵֵ
}

void hash_add_entry(hash_t *hash, void *key, unsigned int key_size,void *value, unsigned int value_size)  // ����hash�ڵ�
{
	if (hash_lookup_entry(hash, key, key_size)) // ���Ȳ���hash���Ƿ�ؼ�ֵΪkey�Ľڵ㣬���������ڵ���ڣ���ʾ�ýڵ��ظ�
	{
		fprintf(stderr, "duplicate hash key\n"); // ��ʾ�ڵ��ظ�
		return;
	} 
	// �����ǰ�ڵ㲻��������������һ���ڵ�
	hash_node_t *node = malloc(sizeof(hash_node_t)); // ����һ��hash�ڵ�ռ�
	node->prev = NULL; 
	node->next = NULL;
	node->key = malloc(key_size); // Ϊ�ؼ�ֵ���ٿռ�
	memcpy(node->key, key, key_size); // ���ؼ�ֵ��䵽�ռ���
	node->value = malloc(value_size); // ����ʵֵ�ռ�
	memcpy(node->value, value, value_size); // ��ʵֵ��䵽�ռ���
	
	hash_node_t **bucket = hash_get_bucket(hash, key); // ��ȡ�ؼ�ֵΪ��*key���Ľڵ����ڵ������׵�ַ
	if (*bucket == NULL) // �����ǰ�����ǿ�������˵����ǰ�����½��ڵ��ǵ�һ���ڵ�
	{
		*bucket = node; // ��ʱ*bucket��ָ���һ���ڵ��ָ�룬
	} 
	else // ������ǿ�������ͷ��
	{
		// ���½ڵ���뵽����ͷ��
		node->next = *bucket; // �½ڵ�ĺ���ָ��ָ��ͷ�ڵ�
		(*bucket)->prev = node; // ͷ�ڵ��ǰ��ָ��ָ���½ڵ�
		*bucket = node; // �½ڵ��Ϊͷ�ڵ�
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
		hash_node_t **bucket = hash_get_bucket(hash, key); // ��ȡ�ؼ�ֵΪ��*key���Ľڵ����ڵ������׵�ַ
		*bucket = node->next;
	} 
	if(node->next)
		node->next->prev = node->prev;
	free(node);
}

hash_node_t** hash_get_bucket(hash_t *hash, void *key) // ��ȡ�ؼ�ֵΪ��*key���Ľڵ����ڵ������׵�ַ������˵�������ҵ�key���ڵ�����
{
	unsigned int bucket = hash->hash_func(hash->buckets, key); // ����hashӳ�亯�������ҹؼ�ֵΪ��*key���Ľڵ�������������
	if (bucket >= hash->buckets) // ���Ŀ��ڵ㲻��hashͰ�У��򱨴�
	{
		fprintf(stderr, "bad bucket lookup\n");
		exit(EXIT_FAILURE);
	}
	return &(hash->nodes[bucket]); // ���عؼ�ֵΪ��*key���Ľڵ����ڵ������׵�ַ
}

hash_node_t* hash_get_node_by_key(hash_t *hash, void *key, unsigned int key_size) // ���ݹؼ�ֵ��ѯ�ڵ�
{
	hash_node_t **bucket = hash_get_bucket(hash, key); // ��ȡ�ؼ�ֵΪ��*key���Ľڵ����ڵ������׵�ַ
	hash_node_t *node = *bucket; // nodeΪָ�������һ���ڵ��ָ��
	if (node == NULL) // �����ǰ����Ϊ�գ���˵����ѯ�ڵ㲻���ڣ�ֱ�ӷ���NULL
	{
		return NULL;
	}
	while (node != NULL && memcmp(node->key, key, key_size) != 0) // ѭ�����ҵ�ǰ�����йؼ�ֵΪ��*key���Ľڵ�
	{
		node = node->next;
	}
	return node; // ���ز��ҵ��Ľڵ�
}