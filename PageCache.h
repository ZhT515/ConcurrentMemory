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

#pragma once
#include "Common.h"

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}
	
	//��ϵͳ����kҳ�ڴ�ҵ���������
	void* SystemAllocPage(size_t k);

	// ����һ���µ�span
	Span* NewSpan(size_t k);

	// ��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);

	//�ͷſ���span�ص�Pagecache�����ϲ����ڵ�span
	void ReleaseSpanToPageCache(Span* span);
private:
	SpanList _spanList[NPAGES];					//��ҳӳ��

	std::unordered_map<PageID, Span*> _idSpanMap;			//ID��span��Ӧ
	//tcmalloc �õĻ�����

	std::recursive_mutex _mtx;

private:										//����
	PageCache()
	{}

	PageCache(const PageCache&) = delete;

	static PageCache _sInst;
};

