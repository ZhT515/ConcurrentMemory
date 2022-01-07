/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021年12月28日14点52分
 * 
 * Content：页缓存：以页为单位申请内存，为中心控制缓存提供大块内存。
 *******************************************************/

#pragma once
#include "Common.h"
//#include "PageMap.h"

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}
	
	//向系统申请k页内存挂到自由链表
	void* SystemAllocPage(size_t k);

	// 申请一个新的span
	Span* NewSpan(size_t k);

	// 获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);

	//释放空闲span回到Pagecache，并合并相邻的span
	void ReleaseSpanToPageCache(Span* span);
private:
	SpanList _spanList[NPAGES];					//按页映射

	std::unordered_map<PageID, Span*> _idSpanMap;			//ID与span对应
	//tcmalloc 用的基数树
	//TCMalloc_PageMap2<32 - PAGE_SHIFT> _idSpanMap;

	std::recursive_mutex _mtx;

private:										//单例
	PageCache()
	{}

	PageCache(const PageCache&) = delete;

	static PageCache _sInst;
};

