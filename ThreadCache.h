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
 * Content: 线程缓存: 解决多线程下高并发运行场景线程之间的锁竞争问题；
 *******************************************************/

#pragma once
#include "Common.h"

class ThreadCache
{
public:
	//申请内存
	void* Allocate(size_t size);

	//释放内存
	void Deallocate(void* ptr, size_t size);

	//从中心缓存中获取对象（慢启动）
	void* FetchFromCentralCache(size_t index, size_t size);

	//释放内存时，链表过长时，回收内存回到中心缓存
	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList _freeLists[NFREELISTS];
};

//通过静态TLS进行线程本地化，每个都对应一个自己的threadcache这样可以避免上锁
//每个线程都会有自己的tls
static __declspec(thread) ThreadCache* tls_threadcache = nullptr;