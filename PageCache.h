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

#pragma once
#include "Common.h"

class PageCache
{
public:
	//向系统申请k页内存挂到自由链表
	void* SystemAllocPage(size_t k);

	// 申请一个新的span
	Span* NewSpan(size_t k);

	// 获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);
private:
	SpanList _spanList[NPAGES];					//按页映射

	std::map<PageID, Span*> _idSpanMap;			//ID与span对应
};

static PageCache pageCache;