/**************
程序功能：二叉搜索树
时间：2021年9月19日15:56:39
**************/
#include<iostream>
#include<vector>
using namespace std;

template<class _Ty> // 前向声明
class BSTree;

template<class _Ty>
class BSTNode // 二叉搜索树节点类型
{
	friend class BSTree<_Ty>; // BSTNode的友元类
public:
	BSTNode(_Ty val = _Ty(), BSTNode* left = nullptr, BSTNode* right = nullptr) // 全缺省默认构造函数，可以用于零初始化
		: data(val), leftChild(left), rightChild(right)
	{}
	~BSTNode()
	{}
private:
	_Ty data; // 数据
	BSTNode* leftChild; // 左子树
	BSTNode* rightChild; // 右子树
}; 

template<class _Ty>
class BSTree // 二叉搜索树类型
{
public:
	BSTree() : root(nullptr)
	{}
	BSTree(vector<_Ty>& nums) : root(nullptr) // 利用vector对树进行初始化。root一定要进行初始化，否则就是野指针
	{
		for (const auto& e : nums)
			Insert(e);
	}
public:
	bool Insert(const _Ty& x) // 插入节点接口
	{
		return Insert(root, x);  // 调用内部保护成员函数
	}
	void Order()const // 中序遍历二叉树
	{
		Order(root);
	}
	BSTNode<_Ty>* Min()const // 求最小值接口
	{
		return Min(root);
	}
	BSTNode<_Ty>* Max()const // 求最大值接口
	{
		return Max(root);
	}
	BSTNode<_Ty>* Search(const _Ty& key)const // 重二叉搜索树中查找某个值
	{
		return Search(root, key);
	}
	BSTNode<_Ty>* Parent(BSTNode<_Ty>* key)const // 查找当前节点的父节点
	{
		return Parent(root, key);
	}

	bool Remove(const _Ty& key) // 删除某个值的节点的接口
	{
		return Remove(root, key);
	}
protected:
	bool Insert(BSTNode<_Ty>*& t, int x) // 递归实现插入一个节点（目前该接口不允许插入重复值）
	{
		if (t == nullptr) // 如果当前节点为第一个节点
		{
			t = new BSTNode<_Ty>(x);
			return true;
		}
		if (x < t->data)
			return Insert(t->leftChild, x);
		else if (x > t->data)
			return Insert(t->rightChild, x);

		return false; // 如果插入失败在返回false
	}
	void Order(BSTNode<_Ty>* t)const // 中序遍历二叉树
	{
		if (t != nullptr)
		{
			Order(t->leftChild);
			cout << t->data << " ";
			Order(t->rightChild);
		}
	}
	BSTNode<_Ty>* Parent(BSTNode<_Ty>* t, BSTNode<_Ty>* key)const // 查找当前节点的父节点（需要与search函数配合使用）
	{
		if (t == nullptr || key == t) // 如果查找的是空树或者查找的是根节点，或者没有查找到目标节点
			return nullptr;
		if (key == t->leftChild || key == t->rightChild) // 如果查找到该节点，则返回该节点的父节点位置
			return t;
		if (key->data < t->data) // 左树查找
			return Parent(t->leftChild, key);
		else if (key->data > t->data) // 右树查找
			return Parent(t->rightChild, key);
	}
	BSTNode<_Ty>* Search(BSTNode<_Ty>* t, const _Ty& key)const // 从二叉搜索树中查找某个值，按值搜索
	{
		if (t == nullptr) // 如果查找的是空树或者目标节点不存在
			return t;
		if (key < t->data) // 如果搜索值小于根节点，则在左子树中查找
			return Search(t->leftChild, key);
		else if (key > t->data) // 如果搜索值大于根节点，则在右子树中查找
			return Search(t->rightChild, key);
		return t; // 如果查找到当前值的位置，则直接返回该位置
	}
	BSTNode<_Ty>* Min(BSTNode<_Ty>* t)const // 求最小值就是查找值最左侧的节点
	{
		while (t && t->leftChild != nullptr) // 如果根节点不空，并且左树不空
			t = t->leftChild;
		return t; // 返回最小值节点的地址
	}
	BSTNode<_Ty>* Max(BSTNode<_Ty>* t)const // 求最大值就是查找值最右侧的节点
	{
		while (t && t->rightChild != nullptr) // 如果根节点不空，并且右树不空
			t = t->rightChild;
		return t; // 返回最大值节点的地址
	}
	bool Remove(BSTNode<_Ty>*& t, const _Ty& key)
	{
		if (t == nullptr) // 如果树为空或没查找到目标节点，则返回false
			return false;
		if (key < t->data) // 小于根节点则应该从左子树中查找删除
			return Remove(t->leftChild, key);
		else if (key > t->data) // 大于根节点则应该从右子树中查找删除
			return Remove(t->rightChild, key);
		else // 如果查找到了目标节点
		{
			BSTNode<_Ty>* p = nullptr;

			//找到了， 删除
			if (t->leftChild == nullptr && t->rightChild != nullptr)// 如果当前节点有右树和左树
			{
				p = t->leftChild; // 保存根节点左树的地址
				while (p->rightChild != nullptr) // 寻找根节点左子树的最大节点
					p = p->rightChild;
				t->data = p->data; // 将左树最大节点的值赋值给根节点
				Remove(t->leftChild, p->data); // 删除根节点左树的最大值
			}
			else // 如果左右子树不全
			{
				p = t;
				if (t->leftChild != nullptr) // 如果左子树不为空
					t = t->leftChild;
				else // 如果右子树不为空或左右子树都不存在
					t = t->rightChild; // 这里有一箭两雕之功效
				delete p;
			}
			return true;
		}
	}
private:
	BSTNode<_Ty>* root;
};
int main()
{
	//vector<int> iv{5,3,4,1,7,8,2,6,0,9};
	//BSTree<int>bst;
	//for (int i = 0; i < iv.size(); ++i)
	//	bst.Insert(iv[i]);

	vector<int> iv{ 50,30,40,10,70,80,2,60,90 };

	BSTree<int> bst(iv);

	BSTNode<int>* pos = bst.Min(); // 求最小值

	BSTNode<int>* p = bst.Search(10); // 查找值为10的节点
	BSTNode<int>* pr = bst.Parent(p); // 查找节点10的父节点

	bst.Remove(50);
	bst.Order();


	return 0;
}