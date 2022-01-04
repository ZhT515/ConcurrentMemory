/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021��12��20��18��19��
 *
 * Content: ������
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

static const size_t MAX_BYTES = 64 * 1024;		//threadcache����ڴ�
static const size_t NFREELISTS = 184;			//Ͱ�ĸ���
static const size_t NPAGES = 129;				//���ҳ��
static const size_t PAGE_SHIFT = 12;			//

inline void*& NextObj(void* obj)			//ȡǰ4/8���ֽڣ�������С����������
{
	return *((void**)obj);			//void**�ٽ����þ�ȡ��ǰ4/8���ֽ� ��ϵͳ�仯��
}

//size_t index(size_t size)			//�����ϣӳ��
//{
//	if (size % 8 == 0)			//�������䶼Ҫ�洢��ͬһ������80-87 ������9
//	{
//		return size / 8 - 1; 
//	}
//	else
//	{
//		return size / 8;
//	}
//}

//������������Ч�ʽϵ�
//�����ϣӳ��
//static ������������
static size_t Index(size_t size)			//�����ϣӳ��
{
	return ((size + (2 ^ 3 - 1)) >> 3) - 1;			//>>3 == /8; ��8+7 = 15 15/8 = 1 1-1 =0....1+7 = 8 1-1 = 0
}

//��������ӳ���ϵ����߿ռ�������
class SizeClass
{
public:
	//�ڴ���Ƭ���������1%-12%����184��Ͱ
	//[0��128]				8byte����            16��  freelist[0,16)
	//[129��1024]			16byte����           56��  freelist[16,72)
	//[1025��8*1024]		128byte����          56��  freelist[72,128)
	//[8*1024+1��64*1024]	1024byte����         56��  freelist[128,184)

	// [1,8]   +7 [8,15]    8
	// [9, 16] +7 [16,23]   16
	//����
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

	//�����Ӧ�Ĺ�ϣ���ο�((size + (2 ^ 3 - 1)) >> 3) - 1;
	static inline size_t  _Index(size_t bytes, size_t align_shift)			
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	static inline size_t  Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);				//ҪС�����ֵ

		static int group_arrary[4] = { 16,56,56,56 };	// ÿ�������ж��ٸ�Ͱ

		if (bytes <= 128)
		{
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024)
		{
			//�����ڵ�ǰ�����λ�ã�Ҫ��ȥǰ��ģ�bytes - 128����������һ�����������
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

		assert(false);			//���˴�˵���������

		return -1;
	}

	//=��������һ�δ����Ļ���ȡ���ٸ�
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;

		//[2,512]
		int num = MAX_BYTES / size;		//��������

		//������
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;

		return num;
	}

	// ����һ����ϵͳ��ȡ����ҳ
	// �������� 8byte - 64KB
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);		
		size_t npage = size * num;					//ȷ���ֽ���

		npage >>= 12;								//ȷ��ҳ��
		if (npage == 0)
			npage = 1;

		return npage;
	}
};

//����������ط���Ҫ����

class FreeList				
{
public:
	void PushRange(void* start, void* end, int n)		//������������,ͷ��n��
	{
		NextObj(end) = _head;
		_head = start;
		_size += n;
	}

	void PopRange(void*& start, void*& end, int n)		//ɾn����start-end��ɾ��������
	{
		start = _head;
			
		for (size_t i = 0; i < n; ++i)
		{

			end = _head;
			_head = NextObj(_head);

		}

		NextObj(end) = nullptr;							//�Ͽ�����
		_size -= n;

	}

	//ͷ��,������ͷ��
	void Push(void* obj)
	{
		NextObj(obj) = _head;
		_head = obj;
		_size += 1;
	}

	//ͷɾ
	void* Pop()
	{
		void* obj = _head;
		_head = NextObj(_head);			//ָ����һ��
		_size -= 1;

		return obj;
	}

	bool Empty()
	{
		return _head == nullptr;
	}

	size_t MaxSize()		//��ȡ_max_size	
	{
		return _max_size;
	}

	void SetMaxSize(size_t n)		//�� _max_size ����
	{
		_max_size = n;
	}

	size_t Size()
	{
		return _size;
	}

private:
	void* _head = nullptr;			//pushʱһ��Ϊͷ�壬���Գ�ʼ��Ϊ��
	size_t _max_size = 1;			//��ʼʱ��󳤶ȣ�����������
	size_t _size = 0;					//���и���
};



/********************SPAN*************************/
//���ڹ�����ҳΪ��λ�Ĵ���ڴ�
//˫����
typedef size_t PageID;

struct Span
{
	size_t _pageId = 0;			//ҳ��
	size_t _n = 0;				//ҳ������

	Span* _next = nullptr;
	Span* _prev = nullptr;

	void* _list = nullptr;				//����ڴ���С��������
	size_t _usecount = 0;				//ʹ�ü��� ==0 ˵�����ж��󶼻�����

	size_t _objsize = 0;					//�г��ĵ�������Ĵ�С
};

//Span������������,������һ��˫���ͷ����
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
		assert(cur != _head);		//����Ϊͷ

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
	std::mutex _mtx;				//��
};

//��ϵͳ�����ڴ�
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage * (1 << PAGE_SHIFT),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// brk mmap��
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
	// sbrk unmmap��
#endif
}