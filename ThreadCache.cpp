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

#include "ThreadCache.h"
#include "CentralCache.h"

 //�����Ļ����л�ȡ������������
void* ThreadCache::FetchFromCentralCache(size_t i, size_t size)
{
	//��ȡһ��������������������,ȡMaxSize��NumMoveSize�н�С��һ����
	size_t batchNum = min(SizeClass::NumMoveSize(size), _freeLists[i].MaxSize());		

	//ȥ���Ļ���ȡbatchNum������
	void* strat = nullptr;
	void* end = nullptr;
	size_t actualNum = centralCache.FetchRangeObj(strat, end, batchNum, size);
	assert(actualNum > 0);		//�Ƿ�ɹ�

	if (actualNum > 1)		//����һ��ʱ������Ĳ�����������
	{
		//NextObj(strat)��strat����һ������strat����
		_freeLists[i].PushRange(NextObj(strat), end, actualNum - 1);
	}

	if (_freeLists[i].MaxSize() == batchNum)
	{
		//���MaxSize�Ǹ�С�ģ���+1��
		_freeLists[i].SetMaxSize(_freeLists[i].MaxSize() + 1);
	}

	return strat;
}

void* ThreadCache::Allocate(size_t size)
{
	size_t i = SizeClass::Index(size);					//ȷ��λ��
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

	//��ǰ���ȴ�����󳤶Ⱦ���Ҫ�ͷ�
	if (_freeLists[i].Size() > _freeLists[i].MaxSize())
	{
		ListTooLong(_freeLists[i], size);
	}
}

void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	size_t batchNum = list.MaxSize();			//�ͷ����
	void* start = nullptr;
	void* end = nullptr;
	list.PopRange(start, end, batchNum);		

	centralCache.ReleaseListToSpans(start, size);
}