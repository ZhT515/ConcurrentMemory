#include "ConcurrentAlloc.h"

void func1()
{
	for (size_t i = 0; i < 10; ++i)
	{
		ConcurrentAlloc(17);
	}
}

void func2()
{
	for (size_t i = 0; i < 20; ++i)
	{
		ConcurrentAlloc(5);
	}
}

void TestThreads()
{
	std::thread t1(func1);
	std::thread t2(func2);


	t1.join();
	t2.join();
}

void TestSizeClass()
{
	cout << SizeClass::Index(1035) << endl;
	cout << SizeClass::Index(1025) << endl;
	cout << SizeClass::Index(1024) << endl;
}

void TestConcurrentAlloc()
{
	void* ptr0 = ConcurrentAlloc(5);
	void* ptr1 = ConcurrentAlloc(8);
	void* ptr2 = ConcurrentAlloc(8);
	void* ptr3 = ConcurrentAlloc(8);

	ConcurrentFree(ptr1);
	ConcurrentFree(ptr2);
	ConcurrentFree(ptr3);
}

void TestBigMemory()
{
	void* ptr1 = ConcurrentAlloc(65 * 1024);
	ConcurrentFree(ptr1);

	void* ptr2 = ConcurrentAlloc(129 * 4 * 1024);
	ConcurrentFree(ptr2);
}

//int main()
//{
////	TestBigMemory();
////
////	TestObjectPool();
////	TestThreads();
////	TestSizeClass();
//	TestConcurrentAlloc();
//
//	return 0;
//}