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


void TestConcurrentAlloc()
{
	void* ptr1 = ConcurrentAlloc(8);
	void* ptr2 = ConcurrentAlloc(8);

}

int main()
{
	TestThreads();
	//TestSizeClass();
	//TestConcurrentAlloc();

	return 0;
}