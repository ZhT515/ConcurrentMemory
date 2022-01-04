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

#include "ThreadCache.h"
#include "CentralCache.h"

 //从中心缓存中获取对象（慢启动）
void* ThreadCache::FetchFromCentralCache(size_t i, size_t size)
{
	//获取一批对象，数量采用慢启动,取MaxSize和NumMoveSize中较小的一个，
	size_t batchNum = min(SizeClass::NumMoveSize(size), _freeLists[i].MaxSize());		

	//去中心缓存取batchNum个对象
	void* strat = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(strat, end, batchNum, SizeClass::RoundUp(size));
	assert(actualNum > 0);		//是否成功

	if (actualNum > 1)		//多于一个时，将多的插入自由链表
	{
		//NextObj(strat)是strat的下一个，将strat返回
		_freeLists[i].PushRange(NextObj(strat), end, actualNum - 1);
	}

	if (_freeLists[i].MaxSize() == batchNum)
	{
		//如果MaxSize是更小的，则+1；
		_freeLists[i].SetMaxSize(_freeLists[i].MaxSize() + 1);
	}

	return strat;
}

void* ThreadCache::Allocate(size_t size)
{
	size_t i = SizeClass::Index(size);					//确定位置
	if (!_freeLists[i].Empty())
	{
		return _freeLists[i].Pop();
	}
	else
	{
		return FetchFromCentralCache(i, size);
	}

	return nullptr;
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	size_t i = SizeClass::Index(size);
	_freeLists[i].Push(ptr);

	//当前长度大于最大长度就需要释放
	if (_freeLists[i].Size() > _freeLists[i].MaxSize())
	{
		ListTooLong(_freeLists[i], size);
	}
}

void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	size_t batchNum = list.MaxSize();			//释放最大
	void* start = nullptr;
	void* end = nullptr;

	list.PopRange(start, end, batchNum);		

	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}