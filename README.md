# ConcurrentMemory
用于替代系统的内存分配相关的函数。实现在多核多线程的环境下，效率较高的处理高并发的内存池。相较于系统的内存分配相关的函数（malloc、free等）减少内存碎片，提高效率，使得平均运行效率该高于malloc，解决多线程场景下在内存申请过程中的锁竞争问题。
.   项目内容：
以定长哈希映射的空闲内存池为基础，使用三层缓存分配结构：
1.	线程缓存：解决多线程下高并发运行场景线程之间的锁竞争问题；
2.	中心控制缓存：负责大块内存切割分配给线程缓存以及回收线程缓存中多余的内存进行合并归还给页缓存；
3.	页缓存：以页为单位申请内存，为中心控制缓存提供大块内存。

.   开发环境：Windows10、C++、VS2019
