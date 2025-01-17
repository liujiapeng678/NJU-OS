
在libc（C语言标准库）中，有许多有趣且实用的函数，以下列举几个有趣的函数及其实现：
1. strlen - 计算字符串长度
功能：strlen 函数用于计算字符串中字符的个数，直到但不包括第一个空字符（\0）。
实现概述：该函数遍历字符串，逐个字符地计数，直到遇到空字符为止。由于它不检查分配给字符串的内存大小，因此它依赖于字符串以空字符正确终止。
原型：size_t strlen(const char *s);
2. strcpy - 字符串复制
功能：strcpy 函数将源字符串（包括空字符）复制到目标字符串中。
实现概述：该函数从源字符串的第一个字符开始复制，直到遇到空字符，然后将目标字符串中的对应位置也设置为空字符。需要注意的是，使用strcpy时要确保目标字符串有足够的空间来存储源字符串，否则可能会导致缓冲区溢出。
原型：char *strcpy(char *dest, const char *src);
3. qsort - 快速排序
功能：虽然qsort不是直接定义在libc的字符串或内存管理部分，但它是C标准库中一个非常重要的排序函数，支持对任意类型的数据进行排序。
实现概述：qsort函数使用快速排序算法，这是一种分而治之的排序算法。它通过选取一个基准元素（pivot），将数组分为两部分，一部分包含小于基准的元素，另一部分包含大于基准的元素，然后递归地对这两部分进行排序。
原型：void qsort(void *base, size_t num, size_t size, int (*compar)(const void *, const void *));
base是指向要排序数组的第一个元素的指针。
num是数组中元素的数量。
size是每个元素的大小（以字节为单位）。
compar是比较函数，用于比较两个元素。
4. malloc 和 free - 动态内存分配
功能：malloc函数用于动态分配指定大小的内存块，而free函数用于释放之前通过malloc（或calloc、realloc）分配的内存。
实现概述：malloc从堆（heap）上分配内存，并返回一个指向该内存的指针。如果分配失败，则返回NULL。free函数接受一个指向先前分配的内存块的指针，并释放该内存块，使其可用于后续的分配。
原型：
void *malloc(size_t size);
void free(void *ptr);
5. printf 和 scanf - 格式化输入输出
功能：printf函数用于向标准输出设备（通常是屏幕）输出格式化的字符串和数据，而scanf函数用于从标准输入设备（如键盘）读取格式化的输入。
实现概述：这两个函数都使用格式化字符串来指定输出或输入的格式。printf根据格式化字符串中的格式说明符输出相应的数据，而scanf则根据格式说明符解析输入数据并将其存储在指定的变量中。
原型：
int printf(const char *format, ...);
int scanf(const char *format, ...);
