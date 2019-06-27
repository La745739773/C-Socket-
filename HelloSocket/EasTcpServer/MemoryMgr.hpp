#ifndef _MEMORYMGR_H_H
#define _MEMORYMGR_H_H
#include<stdlib.h>
#include<assert.h>
#include<mutex>
#ifdef _DEBUG
#include<cstdio>
	#define xPrintf(...) printf(__VA_ARGS__)	//�ɱ��������� 
#else
	#define xPrintf(...)
#endif // _DEBUG
#ifndef _WIN32
typedef long unsigned int size_t;
#endif

//����ڴ�سߴ�
#define MAX_MEMORY_SIZE 128

class MemoryAlloc;
//�ڴ����  ���
class MemoryBlock
{
public:
	MemoryBlock()
	{
		pAlloc = nullptr;
		pNext = nullptr;
	}

public:
	//�ڴ����
	int nID;
	// ���ô���
	int nRef;
	//�����ڴ��
	MemoryAlloc* pAlloc;
	//��һ��λ��
	MemoryBlock* pNext;
	//�Ƿ����ڴ����
	bool bPool;
private:
	//Ԥ��
	char c1;
	char c2;
	char c3;
};


//�ڴ����
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
	//�����ڴ�
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBuff)
		{
			initMemory();
		}
		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHead)	//�ڴ��û�п��п����
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
	//�ͷ��ڴ�
	void freeMem(void* pMem)
	{
		//ƫ��sizeof(MemoryBlock)  
		//��Ϊ�û��õ��ڴ��ַ�� �ڴ��ȥ��ͷ���Ĳ��֣����������Ҫ��ǰƫ��һ���ڴ��ͷ����С�ĵ�ַ
		MemoryBlock* pBlock =  (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);

		if (pBlock->bPool)	//	�ڳ���
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0)
			{
				return;
			}
			pBlock->pNext = _pHead;
			_pHead = pBlock;
		}	
		else  //�ڳ��⣬ϵͳ������ڴ�
		{
			if (--pBlock->nRef != 0)
			{
				return;
			}
			free(pBlock);
		}
	}
	//��ʼ���ڴ��
	void initMemory()
	{
		xPrintf("initMemory: _nSize=%d,_nBlockSize=%d\n", _nSize, _nBlockSize);
		assert(_pBuff == nullptr); //���ԣ�������������� �����˳�
		if (_pBuff)
		{
			return;
		}
		//Ϊ�ڴ����ϵͳ�����ڴ�
		size_t size = _nSize * (_nBlockSize + sizeof(MemoryBlock));
		_pBuff = (char*)malloc(size);
		//��ʼ���ڴ��
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
	MemoryBlock* _pHead;	//�ڴ��ͷ��
	size_t _nBlockSize;	//�ڴ��Ĵ�С
	size_t _nSize;	//�ڴ�������
	char* _pBuff;	//�ڴ���׵�ַ
	std::mutex _mutex;
};


//���������������ʱ������ģ���ʼ����Ա����
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

//�ڴ��������
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
	//�����ڴ�
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
	//�ͷ��ڴ�
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
	//�����ڴ�����ü���
	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		pBlock->nRef++;
	}

private:
	void init_map(int nBegin,int nEnd, MemoryAlloc* pMem_pool)	//��ʼ���ڴ��ӳ������
	{
		for (int i = nBegin; i <= nEnd; i++)
		{
			_szAlloc[i] = pMem_pool;
		}
	}
private:
	MemoryAllocator<64, 4000000> _mem_pool_64;	//���������ʱ�򣬶������ݳ�Ա�ĸ�ֵ
	MemoryAllocator<128, 4000000> _mem_pool_128;
	//MemoryAllocator<256, 1000000> _mem_pool_256;
	//MemoryAllocator<512, 1000000> _mem_pool_512;
	//MemoryAllocator<1024, 1000000> _mem_pool_1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1];		//�ڴ�ص�ӳ������ �洢��ÿ���ֽڶ�Ӧ���ڴ��ָ��
};

//��ָ��ļӼ�����ʱ�������ָ����ָ�Ķ���Ĵ�СΪ�ƶ���λ
#endif // !_MEMORYMGR_H_H
