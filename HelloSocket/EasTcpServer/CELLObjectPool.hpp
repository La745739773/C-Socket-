#ifndef _CELLOBJCETPOOL_HPP_HPP
#define _CELLOBJCETPOOL_HPP_HPP
#include<stdlib.h>
#include<memory>
#include<assert.h>
#ifdef _DEBUG
	#ifndef xPrintf
		#include<cstdio>
		#define xPrintf(...) printf(__VA_ARGS__)	//�ɱ��������� 
	#endif 	
#else
	#ifndef xPrintf
		#define xPrintf(...)
	#endif 
#endif // _DEBUG
//����ؽ���� 
class nodeHeader
{
public:
	//��һ��λ��
	nodeHeader* pNext;
	//�ڴ����
	int nID;
	// ���ô���
	char nRef;
	//�Ƿ����ڴ����
	bool bPool;
private:
	//Ԥ��
	char c1;
	char c2;
};
//�������
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
	//�������
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		nodeHeader* pReturn = nullptr;
		if (nullptr == _pHeader)	//�����û�п��п����
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
	//�ͷŶ���
	void freeObjMem(void* pMem)
	{
		//ƫ��sizeof(MemoryBlock)  
		//��Ϊ�û��õ��ڴ��ַ�� �ڴ��ȥ��ͷ���Ĳ��֣����������Ҫ��ǰƫ��һ���ڴ��ͷ����С�ĵ�ַ
		nodeHeader* pBlock = (nodeHeader*)((char*)pMem - sizeof(nodeHeader));
		xPrintf("freeObjMem: %lx,id=%d\n", pBlock, pBlock->nID);
		assert(1 == pBlock->nRef);
		pBlock->nRef = 0;
		if (pBlock->bPool)	//	�ڳ���
		{
			std::lock_guard<std::mutex> lg(_mutex);
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else  //�ڳ��⣬ϵͳ������ڴ�
		{
			delete[] pBlock;
		}
	}
private:
	//��ʼ�������
	void initObjectPool()
	{
		assert(_pBuff == nullptr); //���ԣ�������������� �����˳�
		if (_pBuff)
		{
			return;
		}
		size_t sz = (sizeof(Type) + sizeof(nodeHeader)) * nPoolsize;
		_pBuff = new char[sz]; //�������ڴ�������ڴ�
						  //��ʼ���ڴ��
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
	char* _pBuff;	//������׵�ַ
	std::mutex _mutex;
};




//����ӿ���
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
		//��̬CELLObjectPool����
		static ClassTypePool sPool;
		return sPool;
	}
};

#endif // !_CELLOBJCETPOOL_HPP_HPP
