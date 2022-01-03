/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021��12��20��18��19��
 *******************************************************/

#include "CentralCache.h"
#include "PageCache.h"

Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//����spanList���е�span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_list)		//��Ϊ�����ҵ���
		{
			return it;
		}

		it = it->_next;
	}

	//spanû���ڴ��ˣ���Ҫ��pagecache;
	Span* span = pageCache.NewSpan(SizeClass::NumMovePage(size));

	//�з֣��ҵ�list
	char* start = (char*)(span->_pageId << PAGE_SHIFT);				//��ʼ��ַ
	char* end = start + (span->_n << PAGE_SHIFT);
	
	while (start < end)
	{
		char* next = start + size;				
		
		//ͷ�壬size�ǵ�λ��С
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
	size_t i = SizeClass::Index(size);		//��������ӳ��
	Span* span = GetOneSpan(_spanLists[i], size);

	//�ҵ�һ���ж����span���ж��ٸ�����
	size_t j = 1;
	start = span->_list;		//ͷ
	void* cur = start;
	void* prev = start;
	while (j <= n && cur != nullptr)
	{	
		prev = cur;						//����
		cur = NextObj(cur);
		j++;
		span->_usecount++;				
	}

	span->_list = cur;					//����ָ��ʣ�µ�
	end = prev;
	NextObj(prev) = nullptr;			//�Ͽ�����

	return j - 1;
}


void CentralCache::ReleaseListToSpans(void* start, size_t byte_size)
{
	size_t i = SizeClass::Index(byte_size);		//���������ĸ�
	while (start)
	{
		void* next = NextObj(start);

		Span* span = pageCache.MapObjectToSpan(start);			//�ҵ�λ��

		//�Ѷ�����뵽span�����list��
		NextObj(start) = span->_list;
		span->_list = start;					//������ͷ��
		span->_usecount--;
		
		//_usecount == 0��ȫ������
		if (span->_usecount == 0)
		{
			_spanLists[i].Erase(span);
			span->_list = nullptr;
			pageCache.ReleaseSpanToPageCache(span);
		}

		start = next;
	}
}