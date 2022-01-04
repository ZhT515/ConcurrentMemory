/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021年12月20日18点19分
 *
 * Content: 单测试
 *******************************************************/

#pragma once
#include <iostream>
#include <execution>
#include <vector>
#include <time.h>
#include <assert.h>
#include <map>
#include <unordered_map>

#include <thread>
#include <mutex>
#include <algorithm>

#ifdef _WIN32
	#include <windows.h>
#else
 // ..
#endif

#ifdef _WIN32
	typedef size_t ADDRES_INT;
#else
	typedef unsigned long long ADDRES_INT;
#endif // _WIN32


// 2 ^ 32 / 2 ^ 12
// 2 ^ 64 / 2 ^ 12
#ifdef _WIN32
	typedef size_t PageID;
#else
	typedef unsigned long long ADDRES_INT;
#endif // _WIN32


using std::cout;
using std::endl;

static const size_t MAX_BYTES = 64 * 1024;		//threadcache最大内存
static const size_t NFREELISTS = 184;			//桶的个数
static const size_t NPAGES = 129;				//最大页数
static const size_t PAGE_SHIFT = 12;			//

inline void*& NextObj(void* obj)			//取前4/8个字节，函数较小，采用内联
{
	return *((void**)obj);			//void**再解引用就取出前4/8个字节 随系统变化。
}

//size_t index(size_t size)			//计算哈希映射
//{
//	if (size % 8 == 0)			//整个区间都要存储到同一个，如80-87 都存在9
//	{
//		return size / 8 - 1; 
//	}
//	else
//	{
//		return size / 8;
//	}
//}

//上述方法除法效率较低
//计算哈希映射
//static 避免链接问题
static size_t Index(size_t size)			//计算哈希映射
{
	return ((size + (2 ^ 3 - 1)) >> 3) - 1;			//>>3 == /8; 如8+7 = 15 15/8 = 1 1-1 =0....1+7 = 8 1-1 = 0
}

//管理对齐和映射关系，提高空间利用率
class SizeClass
{
public:
	//内存碎片问题控制在1%-12%，共184个桶
	//[0，128]				8byte对齐            16个  freelist[0,16)
	//[129，1024]			16byte对齐           56个  freelist[16,72)
	//[1025，8*1024]		128byte对齐          56个  freelist[72,128)
	//[8*1024+1，64*1024]	1024byte对齐         56个  freelist[128,184)

	// [1,8]   +7 [8,15]    8
	// [9, 16] +7 [16,23]   16
	//对齐
	static inline size_t _RoundUp(size_t bytes, size_t align)
	{
		return (((bytes)+align - 1) & ~(align - 1));
	}

	static inline size_t RoundUp(size_t bytes)
	{
		if (bytes <= 128)
		{
			return _RoundUp(bytes, 8);
		}
		else if (bytes <= 1024)
		{
			return _RoundUp(bytes, 16);
		}
		else if (bytes <= 8192)
		{
			return _RoundUp(bytes, 128);
		}
		else if (bytes <= 65536)
		{
			return _RoundUp(bytes, 1024);
		}
		else
		{
			return _RoundUp(bytes, 1 << PAGE_SHIFT);
		}
	}

	//计算对应的哈希，参考((size + (2 ^ 3 - 1)) >> 3) - 1;
	static inline size_t  _Index(size_t bytes, size_t align_shift)			
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	static inline size_t  Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);				//要小于最大值

		static int group_arrary[4] = { 16,56,56,56 };	// 每个区间有多少个桶

		if (bytes <= 128)
		{
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024)
		{
			//计算在当前区间的位置，要减去前面的（bytes - 128），加上上一个区间的总数
			return _Index(bytes - 128, 4) + group_arrary[0];				
		}
		else if (bytes <= 8192)
		{
			return _Index(bytes - 1024, 7) + group_arrary[0] + group_arrary[1];
		}
		else if (bytes <= 65536)
		{
			return _Index(bytes - 8192, 9) + group_arrary[0] + group_arrary[1] + group_arrary[2];
		}

		assert(false);			//到此处说明程序出错

		return -1;
	}

	//=帮助处理一次从中心缓存取多少个
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;

		//[2,512]
		int num = MAX_BYTES / size;		//慢启动。

		//上下限
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;

		return num;
	}

	// 计算一次向系统获取几个页
	// 单个对象 8byte - 64KB
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);		
		size_t npage = size * num;					//确定字节数

		npage >>= 12;								//确定页数
		if (npage == 0)
			npage = 1;

		return npage;
	}
};

//自由链表，多地方需要复用

class FreeList				
{
public:
	void PushRange(void* start, void* end, int n)		//插入自由链表,头插n个
	{
		NextObj(end) = _head;
		_head = start;
		_size += n;
	}

	void PopRange(void*& start, void*& end, int n)		//删n个，start-end被删除的区间
	{
		start = _head;
			
		for (size_t i = 0; i < n; ++i)
		{

			end = _head;
			_head = NextObj(_head);

		}

		NextObj(end) = nullptr;							//断开链接
		_size -= n;

	}

	//头插,单链表头插
	void Push(void* obj)
	{
		NextObj(obj) = _head;
		_head = obj;
		_size += 1;
	}

	//头删
	void* Pop()
	{
		void* obj = _head;
		_head = NextObj(_head);			//指向下一个
		_size -= 1;

		return obj;
	}

	bool Empty()
	{
		return _head == nullptr;
	}

	size_t MaxSize()		//读取_max_size	
	{
		return _max_size;
	}

	void SetMaxSize(size_t n)		//给 _max_size 复制
	{
		_max_size = n;
	}

	size_t Size()
	{
		return _size;
	}

private:
	void* _head = nullptr;			//push时一般为头插，所以初始化为空
	size_t _max_size = 1;			//初始时最大长度，用于慢启动
	size_t _size = 0;					//现有个数
};



/********************SPAN*************************/
//用于管理以页为单位的大块内存
//双链表
typedef size_t PageID;

struct Span
{
	size_t _pageId = 0;			//页号
	size_t _n = 0;				//页的数量

	Span* _next = nullptr;
	Span* _prev = nullptr;

	void* _list = nullptr;				//大块内存切小链接起来
	size_t _usecount = 0;				//使用技术 ==0 说明所有对象都回来了

	size_t _objsize = 0;					//切除的单个对象的大小
};

//Span对象自由链表,本质是一个双向带头链表
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_prev = _head;
		_head->_next = _head;
	}

	void Insert(Span* cur, Span* newspan)
	{
		Span* prev = cur->_prev;

		cur->_prev = newspan;
		prev->_next = newspan;

		newspan->_next = cur;
		newspan->_prev = prev;
	}

	void Erase(Span* cur)
	{
		assert(cur != _head);		//不得为头

		Span* prev = cur->_prev;
		Span* next = cur->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	bool Empty()
	{
		return _head->_next == _head;
	}

	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}

	Span* PopFront()
	{
		Span* span = Begin();
		Erase(span);

		return span;
	}

	void Lock()
	{
		_mtx.lock();
	}

	void UnLock()
	{
		_mtx.unlock();
	}

private:
	Span* _head;

public:
	std::mutex _mtx;				//锁
};

//向系统申请内存
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage * (1 << PAGE_SHIFT),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// brk mmap等
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap等
#endif
}