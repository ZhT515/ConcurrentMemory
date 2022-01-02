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

#include "PageCache.h"

 //向系统申请k页内存挂到自由链表
void* PageCache::SystemAllocPage(size_t k)
{
	return ::SystemAlloc(k);
}

// 申请一个新的span
Span* PageCache::NewSpan(size_t k)
{
	if (!_spanList[k].Empty())			//对应的位置有，则直接申请
	{
		Span* it = _spanList[k].Begin();
		_spanList[k].Erase(it);			//从链表中删除
		return it;
	}

	for (size_t i = k + 1; i < NPAGES; i++)		//寻找大的并切小
	{
		//将大的切小，K返回，i-k挂在自由链表上
		if (!_spanList[i].Empty())		//找到
		{
			Span* span = _spanList[i].Begin();			//取地址
			_spanList[i].Erase(span);					//取下

			Span* splitSpan = new Span;					//切小的
			splitSpan->_pageId = span->_pageId + k;		//span地址+k
			splitSpan->_n = span->_n - k;				//数量-k

			span->_n = k;

			//切下的挂到链中
			_spanList[splitSpan->_n].Insert(_spanList[splitSpan->_n].Begin(), splitSpan);

			return span;
		}
	}

	//向系统申请内存，挂到链表中
	Span* bigSpan = new Span;
	void* memory = SystemAllocPage(NPAGES - 1);			//申请内存
	bigSpan->_pageId = (size_t)memory >> 12;			//添加页号
	bigSpan->_n = NPAGES - 1;

	//在最大处添加，头插
	_spanList[NPAGES - 1].Insert(_spanList[NPAGES - 1].Begin(), bigSpan);

	return NewSpan(k);				//递归，去切小块
}


Span* PageCache::MapObjectToSpan(void* obj) 
{
	PageID id = (ADDRES_INT)obj >> PAGE_SHIFT;			//确定ID

	auto ret = _idSpanMap.find(id);						//查找
	if (ret != _idSpanMap.end())						//合法 返回
	{
		return ret->second;
	}
	else
	{
		assert(false);

		return nullptr;
	}
}