/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021年12月24日15点21分
 * 
 * Content：页缓存：以页为单位申请内存，为中心控制缓存提供大块内存。
 *******************************************************/

#include "PageCache.h"

PageCache PageCache::_sInst;			//单例

 //向系统申请k页内存挂到自由链表
void* PageCache::SystemAllocPage(size_t k)
{
	return ::SystemAlloc(k);
}

// 申请一个新的span
Span* PageCache::NewSpan(size_t k)
{
	std::lock_guard<std::recursive_mutex> lock(_mtx);
	//大于NPAGES直接向系统申请
	if (k >= NPAGES)
	{
		void* ptr = SystemAllocPage(k);
		Span* span = new Span;
		span->_pageId = (ADDRES_INT)ptr >> PAGE_SHIFT;			//添加页号
		span->_n = k;

		_idSpanMap[span->_pageId] = span;

		return span;
	}

	if (!_spanList[k].Empty())			//对应的位置有，则直接申请
	{
		//从链表中删除
		return _spanList[k].PopFront();
	}

	for (size_t i = k + 1; i < NPAGES; ++i)		//寻找大的并切小
	{
		//将大的切小，K返回，i-k挂在自由链表上
		if (!_spanList[i].Empty())		//找到
		{
			//************头切***************//
			//头切，但是地址会乱
			//Span* span = _spanList[i].Begin();			//取地址
			//_spanList[i].Erase(span);					//取下

			//Span* splitSpan = new Span;					//切小的
			//splitSpan->_pageId = span->_pageId + k;		//span地址+k
			//splitSpan->_n = span->_n - k;				//数量-k

			//span->_n = k;

			////切下的挂到链中
			//_spanList[splitSpan->_n].Insert(_spanList[splitSpan->_n].Begin(), splitSpan);

			//return span;

			//************尾切***************//
			//尾切出一个k页的span
			Span* span = _spanList[i].PopFront();					//取下

			Span* splitSpan = new Span;								//切小的
			splitSpan->_pageId = span->_pageId + span->_n - k;		//span地址 + 长度n -k
			splitSpan->_n = k;										//数量k

			//改变切出来span的页号和span的映射关系
			for (PageID i = 0; i < k; ++i)
			{
				_idSpanMap[splitSpan->_pageId + i] = splitSpan;
			}

			span->_n -= k;											//数量-k

			//切下的挂到链中
			_spanList[span->_n].PushFront(span);

			return splitSpan;

		}
	}

	//向系统申请内存，挂到链表中
	Span* bigSpan = new Span;
	void* memory = SystemAllocPage(NPAGES - 1);			//申请内存
	bigSpan->_pageId = (size_t)memory >> 12;			//添加页号
	bigSpan->_n = NPAGES - 1;

	// 按页号和span映射关系建立
	for (PageID i = 0; i < bigSpan->_n; ++i)
	{
		PageID id = bigSpan->_pageId + i;
		_idSpanMap[id] = bigSpan;
	}

	//在最大处添加，头插
	_spanList[NPAGES - 1].Insert(_spanList[NPAGES - 1].Begin(), bigSpan);

	return NewSpan(k);				//递归，去切小块
}


Span* PageCache::MapObjectToSpan(void* obj) 
{
	

	PageID id = (ADDRES_INT)obj >> PAGE_SHIFT;			//确定ID

	//auto ret = _idSpanMap.find(id);						//查找


	//if (ret != _idSpanMap.end())						//合法 返回
	//{

	//	return ret->second;
	//}
	//else
	//{
	//	assert(false);

	//	return nullptr;
	//}
	Span* span = _idSpanMap.get(id);
	if (span != nullptr)
	{
		return span;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

//释放空闲span回到Pagecache，并合并相邻的span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//超出最大值，直接还回系统
	if (span->_n >= NPAGES)
	{
		_idSpanMap.erase(span->_pageId);
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);			//还原地址
		SystemFree(ptr);

		delete span;

		return;
	}


	std::lock_guard<std::recursive_mutex> lock(_mtx);
	//检查前后空闲span页，进行合并，解决内存碎片问题

	//向前合并
	while (1)
	{
		PageID preId = span->_pageId - 1;		//前一个
		//auto ret = _idSpanMap.find(preId);
		//
		////判断是否存在
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}

		//Span* preSpan = ret->second;		//取出SPAN

		Span* preSpan = _idSpanMap.get(preId);
		if (preSpan == nullptr)
		{
			break;
		}

		//判断是否空闲
		if (preSpan->_usecount != 0)
		{
			break;
		}

		//开始合并
		// // 超过128页，不需要合并了
		if (preSpan->_n + span->_n >= NPAGES)
		{
			break;
		}

		//先从span链解下
		_spanList[preSpan->_n].Erase(preSpan);

		span->_pageId = preSpan->_pageId;			//更新ID和N
		span->_n += preSpan->_n;

		//更新映射关系
		for (PageID i = 0; i < preSpan->_n; ++i)
		{
			_idSpanMap[preSpan->_pageId + i] = span;
		}

		delete preSpan;
	}

	//向后合并
	while (1)
	{
		PageID nextId = span->_pageId + span->_n;		//后一个
		//auto ret = _idSpanMap.find(nextId);

		////判断是否存在
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}

		//Span* nextSpan = ret->second;		//取出SPAN

		Span* nextSpan = _idSpanMap.get(nextId);
		if (nextSpan == nullptr)
		{
			break;
		}

		//判断是否空闲
		if (nextSpan->_usecount != 0)
		{
			break;
		}

		//开始合并
		if (nextSpan->_n + span->_n >= NPAGES)
		{
			break;
		}
		//先从span链解下
		_spanList[nextSpan->_n].Erase(nextSpan);

		//更新N
		span->_n += nextSpan->_n;

		//更新映射关系
		for (PageID i = 0; i < nextSpan->_n; ++i)
		{
			_idSpanMap[nextSpan->_pageId + i] = span;
		}

		delete nextSpan;
	}

	//合并出的大span插入对应的链表
	_spanList[span->_n].PushFront(span);
}