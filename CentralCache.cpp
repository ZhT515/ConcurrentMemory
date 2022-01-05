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
 * Content:中心控制缓存：负责大块内存切割分配给线程缓存以及
 回收线程缓存中多余的内存进行合并归还给页缓存；
 *******************************************************/

#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	// 现在spanlist中去找还有内存的span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_list)
		{
			return it;
		}

		it = it->_next;
	}

	// 走到这里代表着span都没有内存了，只能找pagecache
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	// 切分好挂在list中
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	char* end = start + (span->_n << PAGE_SHIFT);
	while (start < end)
	{
		char* next = start + size;
		// 头插
		NextObj(start) = span->_list;
		span->_list = start;

		start = next;
	}

	span->_objsize = size;

	list.PushFront(span);

	return span;
}

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t size)
{
	size_t i = SizeClass::Index(size);

	std::lock_guard<std::mutex> lock(_spanLists[i]._mtx);			//加锁，只对在用的桶加锁锁的守卫

	Span* span = GetOneSpan(_spanLists[i], size);

	// 找到一个有对象的span，有多少给多少
	size_t j = 1;
	start = span->_list;
	void* cur = start;
	void* prev = start;
	while (j <= n && cur != nullptr)
	{
		prev = cur;
		cur = NextObj(cur);
		++j;
		span->_usecount++;
	}

	span->_list = cur;
	end = prev;
	NextObj(prev) = nullptr;

	return j - 1;
}


void CentralCache::ReleaseListToSpans(void* start, size_t byte_size)
{
	size_t i = SizeClass::Index(byte_size);		//计算属于哪个

	std::lock_guard<std::mutex> lock(_spanLists[i]._mtx);

	while (start)
	{
		void* next = NextObj(start);

		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);			//找到位置

		//把对象插入到span管理的list中
		NextObj(start) = span->_list;
		span->_list = start;					//单链表头插
		span->_usecount--;
		
		//_usecount == 0就全回来了
		if (span->_usecount == 0)
		{
			_spanLists[i].Erase(span);
			span->_list = nullptr;
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		}

		start = next;
	}
}