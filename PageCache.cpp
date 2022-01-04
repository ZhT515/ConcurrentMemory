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

#include "PageCache.h"

PageCache PageCache::_sInst;			//����

 //��ϵͳ����kҳ�ڴ�ҵ���������
void* PageCache::SystemAllocPage(size_t k)
{
	return ::SystemAlloc(k);
}

// ����һ���µ�span
Span* PageCache::NewSpan(size_t k)
{
	std::lock_guard<std::recursive_mutex> lock(_mtx);
	//����NPAGESֱ����ϵͳ����
	if (k >= NPAGES)
	{
		void* ptr = SystemAllocPage(k);
		Span* span = new Span;
		span->_pageId = (size_t)ptr >> 12;			//���ҳ��
		span->_n = k;

		_idSpanMap[span->_pageId] = span;

		return span;
	}

	if (!_spanList[k].Empty())			//��Ӧ��λ���У���ֱ������
	{
		//��������ɾ��
		return _spanList[k].PopFront();
	}

	for (size_t i = k + 1; i < NPAGES; i++)		//Ѱ�Ҵ�Ĳ���С
	{
		//�������С��K���أ�i-k��������������
		if (!_spanList[i].Empty())		//�ҵ�
		{
			//************ͷ��***************//
			//ͷ�У����ǵ�ַ����
			//Span* span = _spanList[i].Begin();			//ȡ��ַ
			//_spanList[i].Erase(span);					//ȡ��

			//Span* splitSpan = new Span;					//��С��
			//splitSpan->_pageId = span->_pageId + k;		//span��ַ+k
			//splitSpan->_n = span->_n - k;				//����-k

			//span->_n = k;

			////���µĹҵ�����
			//_spanList[splitSpan->_n].Insert(_spanList[splitSpan->_n].Begin(), splitSpan);

			//return span;

			//************β��***************//
			//β�г�һ��kҳ��span
			Span* span = _spanList[i].PopFront();					//ȡ��

			Span* splitSpan = new Span;								//��С��
			splitSpan->_pageId = span->_pageId + span->_n - k;		//span��ַ + ����n -k
			splitSpan->_n = k;										//����k

			//�ı��г���span��ҳ�ź�span��ӳ���ϵ
			for (PageID i = 0; i < k; ++i)
			{
				_idSpanMap[splitSpan->_pageId] = splitSpan;
			}

			span->_n -= k;											//����-k

			//���µĹҵ�����
			_spanList[span->_n].PushFront(span);

			return splitSpan;

		}
	}

	//��ϵͳ�����ڴ棬�ҵ�������
	Span* bigSpan = new Span;
	void* memory = SystemAllocPage(NPAGES - 1);			//�����ڴ�
	bigSpan->_pageId = (size_t)memory >> 12;			//���ҳ��
	bigSpan->_n = NPAGES - 1;

	// ��ҳ�ź�spanӳ���ϵ����
	for (PageID i = 0; i < bigSpan->_n; ++i)
	{
		PageID id = bigSpan->_pageId + i;
		_idSpanMap[id] = bigSpan;
	}

	//�������ӣ�ͷ��
	_spanList[NPAGES - 1].Insert(_spanList[NPAGES - 1].Begin(), bigSpan);

	return NewSpan(k);				//�ݹ飬ȥ��С��
}


Span* PageCache::MapObjectToSpan(void* obj) 
{
	std::lock_guard<std::recursive_mutex> lock(_mtx);

	PageID id = (ADDRES_INT)obj >> PAGE_SHIFT;			//ȷ��ID

	auto ret = _idSpanMap.find(id);						//����
	if (ret != _idSpanMap.end())						//�Ϸ� ����
	{

		return ret->second;
	}
	else
	{
		assert(false);

		return nullptr;
	}
}

//�ͷſ���span�ص�Pagecache�����ϲ����ڵ�span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//�������ֵ��ֱ�ӻ���ϵͳ
	if (span->_n >= NPAGES)
	{
		_idSpanMap.erase(span->_pageId);
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);			//��ԭ��ַ
		SystemFree(ptr);

		delete span;

		return;
	}

	//���ǰ�����spanҳ�����кϲ�������ڴ���Ƭ����

	//��ǰ�ϲ�
	while (1)
	{
		PageID preId = span->_pageId - 1;		//ǰһ��
		auto ret = _idSpanMap.find(preId);
		
		//�ж��Ƿ����
		if (ret == _idSpanMap.end())
		{
			break;
		}

		Span* preSpan = ret->second;		//ȡ��SPAN

		//�ж��Ƿ����
		if (preSpan->_usecount != 0)
		{
			break;
		}

		//��ʼ�ϲ�
		//�ȴ�span������
		_spanList[preSpan->_n].Erase(preSpan);

		span->_pageId = preSpan->_pageId;			//����ID��N
		span->_n += preSpan->_n;

		//����ӳ���ϵ
		for (PageID i = 0; i < preSpan->_n; ++i)
		{
			_idSpanMap[preSpan->_pageId + i] = span;
		}

		delete preSpan;
	}

	//���ϲ�
	while (1)
	{
		PageID nextId = span->_pageId + span->_n;		//��һ��
		auto ret = _idSpanMap.find(nextId);

		//�ж��Ƿ����
		if (ret == _idSpanMap.end())
		{
			break;
		}

		Span* nextSpan = ret->second;		//ȡ��SPAN

		//�ж��Ƿ����
		if (nextSpan->_usecount != 0)
		{
			break;
		}

		//��ʼ�ϲ�
		//�ȴ�span������
		_spanList[nextSpan->_n].Erase(nextSpan);

		//����N
		span->_n += nextSpan->_n;

		//����ӳ���ϵ
		for (PageID i = 0; i < nextSpan->_n; ++i)
		{
			_idSpanMap[nextSpan->_pageId + i] = span;
		}

		delete nextSpan;
	}

	//�ϲ����Ĵ�span�����Ӧ������
	_spanList[span->_n].PushFront(span);
}