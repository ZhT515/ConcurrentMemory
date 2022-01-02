/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021年12月20日18点19分
 *******************************************************/

#include "CentralCache.h"
#include "PageCache.h"

Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//先找spanList现有的span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_list)		//不为空则找到了
		{
			return it;
		}

		it = it->_next;
	}

	//span没有内存了，需要找pagecache;
	Span* span = pageCache.NewSpan(SizeClass::NumMovePage(size));

	return span;
}


size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t size)
{
	size_t i = SizeClass::Index(size);		//计算具体的映射
	Span* span = GetOneSpan(_spanLists[i], size);

	//找到一个有对象的span，有多少给多少
	size_t j = 1;
	start = span->_list;		//头
	void* cur = start;
	void* prev = start;
	while (j <= n && cur != nullptr)
	{	
		prev = cur;						//迭代
		cur = NextObj(cur);
		j++;
		span->_usecount++;				
	}

	span->_list = cur;					//重新指向剩下的
	end = prev;
	NextObj(prev) = nullptr;			//断开连接

	return j - 1;
}


void* CentralCache::ReleaseListToSpans(void* start, size_t byte_size)
{
	size_t i = SizeClass::Index(byte_size);		//计算属于哪个
	while (start)
	{
		void* next = NextObj(start);
		Span* span = pageCache.MapObjectToSpan(start);			//找到位置




		start = next;
	}
}