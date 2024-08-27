#include <common.h>
#include <pmm.h>
#include <stdint.h>

int types[SLAB_TYPE_NR] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
void * pmm_current = NULL; // 当前的堆区的空闲开始地址， 为变化量
lock_t pmm_lk = LOCK_INIT(); // pmm_lk 用于维护pmm_current变量
page_t* page_list[SLAB_TYPE_NR]; // page_list[i]为页面串联链表，存储类型为 i 的页面
lock_t global_lk[SLAB_TYPE_NR]; // 每一种类型的页面对应的锁，用于维护page_list

/* 这两个是初始化之后就固定不变的量 */
void * pmm_end;
void * pmm_start;

// 返回 2^i, 满足 2^i >= size
static size_t align_size(size_t size) {
  size_t ret = 1;
  while(ret < size) ret <<= 1;
  ret = (ret >types[0]) ? ret : types[0]; // 至少对齐到 16
  return ret;
}

page_t * new_page (int size) {
/*  创建一个新的页面， 并进行相关初始化
    [pmm_currennt,  pmm_current + PGSZ) 将作为返回的页面 */

  lock(&pmm_lk); // pmm_lk 用于更新pmm_current变量
  pmm_current = (void*)ALIGN((uintptr_t)pmm_current, PGSZ);
  page_t * newpage = (page_t *) pmm_current;
  pmm_current = pmm_current + PGSZ; // 将pmm_current进行修改
  unlock(&pmm_lk);

  assert((void *)pmm_current <= pmm_end); // 不能超过pmm_end
  assert((uintptr_t)newpage % PGSZ == 0); // 申请到的页面要求与 8 KiB 对齐
  newpage->lk = LOCK_INIT(); 
  newpage->next = NULL; // 下一个页面为空
  newpage->size = size;

  newpage->header = (void*)ALIGN((uintptr_t)&newpage->header, size);
  newpage->header->addr = newpage->header;
  newpage->next = NULL;

  uintptr_t ptr;
  // To be Test
  while(TRUE) {
    ptr = ALIGN((uintptr_t)newpage->header->addr + size, size);
    // [ptr, ptr + size) 将会被分配出去
    if(ptr + size > (uintptr_t)newpage + PGSZ)break;
    list * item = (list *)ptr;
    item->addr = item; // 为当前可以用的slab的地址
    item->next = newpage->header;
    newpage->header = item;
  }
  return newpage;
}

static void * Big_Mem(size_t size) {
  Log("big mem:%d", size);
  lock(&pmm_lk);
  pmm_current = (void*)ALIGN((uintptr_t)pmm_current, size);
  void * ret = pmm_current;
  pmm_current = pmm_current + size;
  assert(pmm_current <= pmm_end);
  unlock(&pmm_lk);
  memset(ret, 0, size);
  panic_on(ret == NULL, "kalloc NULL");
  return ret;
}

static void *kalloc(size_t size) {
  size = align_size(size); // 对齐
  void * ret = NULL;
  int type; // 得到当前的对齐类型
  for(type = 0; type < SLAB_TYPE_NR; type++) {
    if(size == types[type]) break;
  }

  if(type == SLAB_TYPE_NR) { // 说明当前的分配大于 4096 B (4 KiB)
    return Big_Mem(size);
  }
  /*
  如果当前的页面的header不为空，那么直接返回header的头节点，同时修改header
  如果当前的页面的header为空，那么说明当前页面已经满了，需要切换到同类型的下一个页面

  回收的时候，根据地址判断属于哪一个页面，然后将回收内容插入该页面的header
  */
  page_t * current = page_list[type]; // 获得第 i 种类型的链表
  assert(current != NULL);  
  while(current != NULL) {
    lock(&current->lk);
    if(current->header != NULL) {
      ret = current->header->addr;
      current->header = current->header->next;
      unlock(&current->lk);
      assert((uintptr_t)ret + size <= (uintptr_t)current + PGSZ);
      break;
    } else{
      unlock(&current->lk);
      current = current->next;
    }
  }

  if(current == NULL) {
    page_t *pg = new_page(size);
    lock(&global_lk[type]);
    pg->next = page_list[type];
    page_list[type] = pg;
    unlock(&global_lk[type]);
    // 在新申请到的页面内返回slab
    lock(&pg->lk);
    ret = pg->header->addr;
    pg->header = pg->header->next;
    unlock(&pg->lk);
    assert((uintptr_t)ret + size <= (uintptr_t)pg + PGSZ);
  }
  assert((uintptr_t)ret % size == 0);
  panic_on(ret == NULL, "alloc NULL!");
  // memset(ret, 0, size); // for debugging
  return ret;
}

static void kfree(void *ptr) {
  // 大内存会出现回收出现问题的情况，尚未处理
  assert(ptr);
  // 需要知道属于哪一种页面
  page_t * pg = (page_t *)((uintptr_t)ptr & ~(PGSZ - 1));
  int sz = pg->size;
  lock(&(pg->lk));
  list * item = (list *)ptr;
  memset(ptr, 0, sz);
  item->addr = ptr;
  item->next = pg->header;
  pg->header = item;
  unlock(&(pg->lk));
}


#ifndef TEST
// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init() {
  // uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  // printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  heap.start = (void *)ALIGN((uintptr_t)(heap.start), PGSZ); 
  pmm_start = heap.start;  pmm_end = heap.end; // 这两个量之后将会保持恒定
  assert((uintptr_t)pmm_start % PGSZ == 0);
  debug("pmm_start: %p, pmm_end: %p, total page number = %d\n", pmm_start, pmm_end, (pmm_end - pmm_start) / (4096));
  pmm_current = pmm_start; // 动态变化的量
  pmm_lk = LOCK_INIT();
  for(int i = 0; i < SLAB_TYPE_NR; i++) {  // 为每一种slab类型分配一个初始化页面
    page_t * pg = new_page(types[i]);
    page_list[i] = pg;
  }
  assert(pmm_current ==  heap.start +  SLAB_TYPE_NR * PGSZ);
}
#else
// 测试代码的 pmm_init ()
#define HEAP_SIZE (125 << 20)
Area heap = {};
static void pmm_init() {
  char *ptr  = malloc(HEAP_SIZE);
  heap.start = ptr; 
  heap.end   = ptr + HEAP_SIZE; 
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
  heap.start = ALIGN((uintptr_t)(heap.start), PGSZ); 
  pmm_start = heap.start;  pmm_end = heap.end; // 这两个量之后将会保持恒定
  assert((uintptr_t)pmm_start % PGSZ == 0);
  debug("pmm_start: %p, pmm_end: %p\n", pmm_start, pmm_end);
  pmm_current = pmm_start; // 动态变化的量
  pmm_lk = LOCK_INIT();
  for(int i = 0; i < SLAB_TYPE_NR; i++) {  // 为每一种slab类型分配一个初始化页面
    page_t * pg = new_page(types[i]);
    page_list[i] = pg;
  }
  assert(pmm_current ==  heap.start +  SLAB_TYPE_NR * PGSZ);
}
#endif


static void *kalloc_safe(int size) {
  bool enable = ienabled();
  iset(false);
  void *ret = kalloc(size);
  if (enable) iset(true);
  return ret;
}

static void kfree_safe(void *ptr) {
  int enable = ienabled();
  iset(false);
  kfree(ptr);
  if (enable) iset(true);
}


MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc_safe,
  .free  = kfree_safe,
};

