/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021年12月30日22点19分
 * 
 * Content：中心控制缓存：负责大块内存切割分配给线程缓存以及
 回收线程缓存中多余的内存进行合并归还给页缓存；
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

	//从中心缓存获取一定数量的对象给tread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	//从SpanList或Page cache获取一个Span
	Span* GetOneSpan(SpanList&list, size_t size);

	//将一定数量的对象释放到span中
	void ReleaseListToSpans(void* start, size_t byte_size);

private:
	SpanList _spanLists[NFREELISTS];		//按对齐方式映射

private:
	CentralCache()
	{}

	CentralCache(const CentralCache&) = delete;

	static CentralCache _sInst;		//单例
};


