/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021��12��30��22��19��
 * 
 * Content�����Ŀ��ƻ��棺�������ڴ��и������̻߳����Լ�
 �����̻߳����ж�����ڴ���кϲ��黹��ҳ���棻
 *******************************************************/

#pragma once

#include "Common.h"

class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	//�����Ļ����ȡһ�������Ķ����tread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	//��SpanList��Page cache��ȡһ��Span
	Span* GetOneSpan(SpanList&list, size_t size);

	//��һ�������Ķ����ͷŵ�span��
	void ReleaseListToSpans(void* start, size_t byte_size);

private:
	SpanList _spanLists[NFREELISTS];		//�����뷽ʽӳ��

private:
	CentralCache()
	{}

	CentralCache(const CentralCache&) = delete;

	static CentralCache _sInst;		//����
};


