/*******************************************************
 * This file is part of ConcurrentMemory.
 *
 * Licensed under the GNU General Public License v3.0;
 * you may not use this file except in compliance with the License.
 *
 * Author: Zhang Tong (zhangtongcpp@163.com)
 *
 * Last Modified: 2021��12��20��18��19��
 *
 * Content: ����ӿڣ�ֻ����2���ӿڣ���������ͷ�
 *******************************************************/

#pragma once

#include "Common.h"
#include "ThreadCache.h"

//����
void* ConcurrentAlloc(size_t size)		
{
	if (size > MAX_BYTES)
	{
		// PageCache
	}
	else
	{
		if (tls_threadcache == nullptr)		//�����ǰ���߳���ThreadC
		{
			tls_threadcache = new ThreadCache;
		}

		cout << tls_threadcache << endl;

		return tls_threadcache->Allocate(size);
	}


}

//�ͷ�
void ConcurrentFree(void* ptr, size_t size)
{
	assert(tls_threadcache);

	if (size > MAX_BYTES)
	{
		// PageCache
	}
	else
	{
		tls_threadcache->Deallocate(ptr, size);
	}
}