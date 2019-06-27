#ifndef _MEMORYMGR_H_H
#define _MEMORYMGR_H_H
#include<stdlib.h>
#include<assert.h>
#include<mutex>
#ifdef _DEBUG
#include<cstdio>
	#define xPrintf(...) printf(__VA_ARGS__)	//可变数量参数 
#else
	#define xPrintf(...)
#endif // _DEBUG
#ifndef _WIN32
typedef long unsigned int size_t;
#endif

//最大内存池尺寸
#define MAX_MEMORY_SIZE 128

class MemoryAlloc;
//内存块类  结点
class MemoryBlock
{
public:
	MemoryBlock()
	{
		pAlloc = nullptr;
		pNext = nullptr;
	}

public:
	//内存块编号
	int nID;
	// 引用次数
	int nRef;
	//所属内存池
	MemoryAlloc* pAlloc;
	//下一块位置
	MemoryBlock* pNext;
	//是否在内存池中
	bool bPool;
private:
	//预留
	char c1;
	char c2;
	char c3;
};


//内存池类
class MemoryAlloc 
{
public:
	MemoryAlloc()
	{
		_pBuff = nullptr;
		_pHead = nullptr;
		_nBlockSize = 0;
		_nSize = 0;
		xPrintf("MemoryAlloc\n");
	}
	~MemoryAlloc()
	{
		if(_pBuff)
			free(_pBuff);
		_pBuff = nullptr;
		_pHead = nullptr;
		_nBlockSize = 0;
		_nSize = 0;
	}
	//申请内存
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBuff)
		{
			initMemory();
		}
		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHead)	//内存池没有空闲块可用
		{
			pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			printf("allocMem: %lx,id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		}
		else
		{
			pReturn = _pHead;
			_pHead = _pHead->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef++;
		}
		xPrintf("allocMem: %lx,id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(MemoryBlock));
	}
	//释放内存
	void freeMem(void* pMem)
	{
		//偏移sizeof(MemoryBlock)  
		//因为用户用的内存地址是 内存块去掉头部的部分，因此这里需要向前偏移一个内存块头部大小的地址
		MemoryBlock* pBlock =  (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);

		if (pBlock->bPool)	//	在池内
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0)
			{
				return;
			}
			pBlock->pNext = _pHead;
			_pHead = pBlock;
		}	
		else  //在池外，系统申请的内存
		{
			if (--pBlock->nRef != 0)
			{
				return;
			}
			free(pBlock);
		}
	}
	//初始化内存池
	void initMemory()
	{
		xPrintf("initMemory: _nSize=%d,_nBlockSize=%d\n", _nSize, _nBlockSize);
		assert(_pBuff == nullptr); //断言，如果不满足条件 程序退出
		if (_pBuff)
		{
			return;
		}
		//为内存池向系统申请内存
		size_t size = _nSize * (_nBlockSize + sizeof(MemoryBlock));
		_pBuff = (char*)malloc(size);
		//初始化内存池
		_pHead = (MemoryBlock*)_pBuff;
		_pHead->nID = 0;
		_pHead->bPool = true;
		_pHead->nRef = 0;
		_pHead->pAlloc = this;
		_pHead->pNext = nullptr;
		//MemoryBlock* pTemp = _pHead;
		MemoryBlock* pTemp1 = _pHead;

		for (size_t n = 1; n < _nSize; n++)
		{
			MemoryBlock* pTemp2 = (MemoryBlock*)((char*)_pBuff + (n * (sizeof(MemoryBlock) + _nBlockSize)));
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pAlloc = this;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
public:
	MemoryBlock* _pHead;	//内存块头部
	size_t _nBlockSize;	//内存块的大小
	size_t _nSize;	//内存块的数量
	char* _pBuff;	//内存池首地址
	std::mutex _mutex;
};


//便于在声明类对象时，利用模板初始化成员变量
template<size_t nBlockSize, size_t nSize>
class MemoryAllocator : public MemoryAlloc
{
public:
	MemoryAllocator()
	{
		//8 4   61/8=7  61%8=5
		const size_t n = sizeof(void*);
		//(7*8)+8 
		_nSize = nSize;
		_nBlockSize = (nBlockSize / n) * n + (nSize % n ? n : 0);
		//size_t n = sizeof(void*);
		//if (nBlockSize - (nBlockSize / n) * n == 0)
		//{
		//	_nBlockSize = nBlockSize;
		//	_nSize = nSize;
		//}
		//else
		//{
		//	_nBlockSize = (nBlockSize / n + 1) * n;
		//	_nSize = nSize;
		//}
	}
};

//内存管理工具类
class MemoryMgr
{
private:
	MemoryMgr()
	{
		init_map(0, 64, &_mem_pool_64);
		init_map(65,128, &_mem_pool_128);
		//init_map(129,256, &_mem_pool_256);
		//init_map(257, 512, &_mem_pool_512);
		//init_map(513, 1024, &_mem_pool_1024);
		xPrintf("MemoryMgr\n");
	}
	~MemoryMgr()
	{

	}
public:
	static MemoryMgr& Instance()
	{
		static MemoryMgr mgr;
		return mgr;
	}
	//申请内存
	void* allocMem(size_t nSize)
	{
		if (nSize > MAX_MEMORY_SIZE)
		{
			MemoryBlock* pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			xPrintf("allocMem: %lx,id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
			return ((char*)pReturn + sizeof(MemoryBlock));
		}
		else
		{
			return _szAlloc[nSize]->allocMemory(nSize);
		}
		return nullptr;
	}
	//释放内存
	void freeMem(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		xPrintf("freeMem: %lx,id=%d\n", pBlock, pBlock->nID);
		//printf("freeMem: %lx,id=%d\n", pBlock, pBlock->nID);
		if (pBlock->bPool == true)
		{
			pBlock->pAlloc->freeMem(pMem);
		}
		else
		{
			xPrintf("freeMem: %lx,id=%d\n", pBlock, pBlock->nID);
			if (--pBlock->nRef == 0)
			{
				free(pBlock);
			}
		}
	}
	//增加内存块引用计数
	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		pBlock->nRef++;
	}

private:
	void init_map(int nBegin,int nEnd, MemoryAlloc* pMem_pool)	//初始化内存池映射数组
	{
		for (int i = nBegin; i <= nEnd; i++)
		{
			_szAlloc[i] = pMem_pool;
		}
	}
private:
	MemoryAllocator<64, 4000000> _mem_pool_64;	//声明对象的时候，对于数据成员的赋值
	MemoryAllocator<128, 4000000> _mem_pool_128;
	//MemoryAllocator<256, 1000000> _mem_pool_256;
	//MemoryAllocator<512, 1000000> _mem_pool_512;
	//MemoryAllocator<1024, 1000000> _mem_pool_1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1];		//内存池的映射数组 存储了每个字节对应的内存池指针
};

//做指针的加减运算时，会根据指针所指的对象的大小为移动单位
#endif // !_MEMORYMGR_H_H
