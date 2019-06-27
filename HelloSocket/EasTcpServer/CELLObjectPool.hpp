#ifndef _CELLOBJCETPOOL_HPP_HPP
#define _CELLOBJCETPOOL_HPP_HPP
#include<stdlib.h>
#include<memory>
#include<assert.h>
#ifdef _DEBUG
	#ifndef xPrintf
		#include<cstdio>
		#define xPrintf(...) printf(__VA_ARGS__)	//可变数量参数 
	#endif 	
#else
	#ifndef xPrintf
		#define xPrintf(...)
	#endif 
#endif // _DEBUG
//对象池结点类 
class nodeHeader
{
public:
	//下一块位置
	nodeHeader* pNext;
	//内存块编号
	int nID;
	// 引用次数
	char nRef;
	//是否在内存池中
	bool bPool;
private:
	//预留
	char c1;
	char c2;
};
//对象池类
template<class Type,size_t nPoolsize>
class CELLObjectPool
{
public:
	CELLObjectPool()
	{
		_pBuff = nullptr;
		_pHeader = nullptr;
		initObjectPool();
	}
	~CELLObjectPool()
	{
		if (_pBuff)
			delete[] _pBuff;
		_pBuff = nullptr;
		_pHeader = nullptr;
	}
	//申请对象
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		nodeHeader* pReturn = nullptr;
		if (nullptr == _pHeader)	//对象池没有空闲块可用
		{
			pReturn = (nodeHeader*)new char[(sizeof(Type) + sizeof(nodeHeader))];
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pNext = nullptr;
		}
		else
		{
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef++;
		}
		xPrintf("allocObjMem: %lx,id=%d,size=%d\n", pReturn, pReturn->nID,nSize);
		return ((char*)pReturn + sizeof(nodeHeader));
	}
	//释放对象
	void freeObjMem(void* pMem)
	{
		//偏移sizeof(MemoryBlock)  
		//因为用户用的内存地址是 内存块去掉头部的部分，因此这里需要向前偏移一个内存块头部大小的地址
		nodeHeader* pBlock = (nodeHeader*)((char*)pMem - sizeof(nodeHeader));
		xPrintf("freeObjMem: %lx,id=%d\n", pBlock, pBlock->nID);
		assert(1 == pBlock->nRef);
		pBlock->nRef = 0;
		if (pBlock->bPool)	//	在池内
		{
			std::lock_guard<std::mutex> lg(_mutex);
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else  //在池外，系统申请的内存
		{
			delete[] pBlock;
		}
	}
private:
	//初始化对象池
	void initObjectPool()
	{
		assert(_pBuff == nullptr); //断言，如果不满足条件 程序退出
		if (_pBuff)
		{
			return;
		}
		size_t sz = (sizeof(Type) + sizeof(nodeHeader)) * nPoolsize;
		_pBuff = new char[sz]; //优先向内存池申请内存
						  //初始化内存池
		_pHeader = (nodeHeader*)_pBuff;
		_pHeader->nID = 0;
		_pHeader->bPool = true;
		_pHeader->nRef = 0;
		_pHeader->pNext = nullptr;
		nodeHeader* pTemp1 = _pHeader;
		size_t realSize = sizeof(nodeHeader) + sizeof(Type);
		for (size_t n = 1; n < nPoolsize; n++)
		{
			nodeHeader* pTemp2 = (nodeHeader*)((char*)_pBuff + (n * realSize));
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
private:
	nodeHeader* _pHeader;
	char* _pBuff;	//对象池首地址
	std::mutex _mutex;
};




//对外接口类
template<class Type, size_t nPoolsize>
class ObjectPoolBase
{
public:
	void* operator new(size_t nSize)
	{
		return ObjectPool().allocMemory(nSize);
	}
	void* operator new[](size_t nSize)
	{
		ObjectPool().allocMemory(nSize);
	}
	void operator delete(void* p)
	{
		ObjectPool().freeObjMem(p);
	}
	void operator delete[](void* p)
	{
		ObjectPool().freeObjMem(p);
	}
	template<typename ...Args>
	static Type* createObject(Args ...args)
	{
		Type* obj = new Type(args...);
		return obj;
	}
	static Type* createObject()
	{
		Type* obj = new Type();
		return obj;
	}
	static void destroyObject(Type* obj)
	{
		delete obj;
	}
private:
	typedef CELLObjectPool<Type, nPoolsize> ClassTypePool;
	static ClassTypePool& ObjectPool ()
	{
		//静态CELLObjectPool对象
		static ClassTypePool sPool;
		return sPool;
	}
};

#endif // !_CELLOBJCETPOOL_HPP_HPP
