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

 //��ϵͳ����kҳ�ڴ�ҵ���������
void* PageCache::SystemAllocPage(size_t k)
{
	return ::SystemAlloc(k);
}

// ����һ���µ�span
Span* PageCache::NewSpan(size_t k)
{
	if (!_spanList[k].Empty())			//��Ӧ��λ���У���ֱ������
	{
		Span* it = _spanList[k].Begin();
		_spanList[k].Erase(it);			//��������ɾ��
		return it;
	}

	for (size_t i = k + 1; i < NPAGES; i++)		//Ѱ�Ҵ�Ĳ���С
	{
		//�������С��K���أ�i-k��������������
		if (!_spanList[i].Empty())		//�ҵ�
		{
			Span* span = _spanList[i].Begin();			//ȡ��ַ
			_spanList[i].Erase(span);					//ȡ��

			Span* splitSpan = new Span;					//��С��
			splitSpan->_pageId = span->_pageId + k;		//span��ַ+k
			splitSpan->_n = span->_n - k;				//����-k

			span->_n = k;

			//���µĹҵ�����
			_spanList[splitSpan->_n].Insert(_spanList[splitSpan->_n].Begin(), splitSpan);

			return span;
		}
	}

	//��ϵͳ�����ڴ棬�ҵ�������
	Span* bigSpan = new Span;
	void* memory = SystemAllocPage(NPAGES - 1);			//�����ڴ�
	bigSpan->_pageId = (size_t)memory >> 12;			//���ҳ��
	bigSpan->_n = NPAGES - 1;

	//�������ӣ�ͷ��
	_spanList[NPAGES - 1].Insert(_spanList[NPAGES - 1].Begin(), bigSpan);

	return NewSpan(k);				//�ݹ飬ȥ��С��
}


Span* PageCache::MapObjectToSpan(void* obj) 
{
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