/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021年12月30日18点19分
 *******************************************************/

#pragma once

#include "Common.h"

class CentralCache
{
public:
	//从中心缓存获取一定数量的对象给tread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	//从SpanList或Page cache获取一个Span
	Span* GetOneSpan(SpanList&list, size_t size);

	//将一定数量的对象释放到span中
	void ReleaseListToSpans(void* start, size_t byte_size);

private:
	SpanList _spanLists[NFREELISTS];		//按对齐方式映射
};

static CentralCache centralCache;		//代替单例
