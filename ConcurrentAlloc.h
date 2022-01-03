/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021年12月20日18点19分
 *
 * Content: 对外接口，只包含2个接口，即申请和释放
 *******************************************************/

#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//申请
void* ConcurrentAlloc(size_t size)		
{
	if (size > MAX_BYTES)
	{
		// PageCache
	}
	else
	{
		if (tls_threadcache == nullptr)		//如果当前的线程无ThreadC
		{
			tls_threadcache = new ThreadCache;
		}

		//cout << tls_threadcache << endl;

		return tls_threadcache->Allocate(size);
	}


}

//释放
void ConcurrentFree(void* ptr)
{
	Span* span = pageCache.MapObjectToSpan(ptr);		//获取映射
	size_t size = span->_objsize;					

	if (size > MAX_BYTES)
	{
		// PageCache
		pageCache.ReleaseSpanToPageCache(span);
	}
	else
	{
		assert(tls_threadcache);

		tls_threadcache->Deallocate(ptr, size);
	}
}