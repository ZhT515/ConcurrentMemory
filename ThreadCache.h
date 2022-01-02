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
 * Content: �̻߳���: ������߳��¸߲������г����߳�֮������������⣻
 *******************************************************/

#pragma once
#include "Common.h"

class ThreadCache
{
public:
	//�����ڴ�
	void* Allocate(size_t size);

	//�ͷ��ڴ�
	void Deallocate(void* ptr, size_t size);

	//�����Ļ����л�ȡ������������
	void* FetchFromCentralCache(size_t index, size_t size);

	//�ͷ��ڴ�ʱ���������ʱ�������ڴ�ص����Ļ���
	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList _freeLists[16];
};

//ͨ����̬TLS�����̱߳��ػ���ÿ������Ӧһ���Լ���threadcache�������Ա�������
//ÿ���̶߳������Լ���tls
static __declspec(thread) ThreadCache* tls_threadcache = nullptr;