#ifndef _ALLOCATOR_H_H
#define _ALLOCATOR_H_H
#ifndef _WIN32
typedef long unsigned int size_t;
#endif

#ifdef _WIN32
void* operator new[](size_t nSize);
void* operator new(size_t nSize);
void operator delete(void* p);
void operator delete[](void* p);
void* mem_alloc(size_t nSize);
void mem_free(void* p);
#else
void* operator new[](size_t nSize);
void* operator new(size_t nSize);
void operator delete(void* p) noexcept;
void operator delete[](void* p) noexcept;
#endif 
#endif // !_ALLOC_H_H
