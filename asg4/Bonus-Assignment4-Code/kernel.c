#include "kernel.h"

/*
  1. Check if a free process slot exists and if the there's enough free space (check allocated_pages).
  2. Alloc space for page_table (the size of it depends on how many pages you need) and update allocated_pages.
  3. The mapping to kernel-managed memory is not built up, all the PFN should be set to -1 and present byte to 0.
  4. After creating the process, update global state: allocated_pages
  5. Return a pid (the index in MMStruct array) which is >= 0 when success, -1 when failure in any above step.
  @param[in]  kernel    kernel simulator
  @param[in]  size      number of needed bytes
  @return     pid       process id when success, -1 when failure
*/
int proc_create_vm(struct Kernel* kernel, int size) {
  /* Fill Your Code Below */
  
  // check if size meets the limit
  if(size > VIRTUAL_SPACE_SIZE || size < 0) 
    return -1;

  // check if there is enough free space
  if(KERNEL_SPACE_SIZE - kernel->allocated_pages*PAGE_SIZE < size) 
    return -1;

  // calculate number of pages
  int page = size % PAGE_SIZE == 0 ?
             size / PAGE_SIZE : 
             size / PAGE_SIZE + 1;

  // alloc pid
  int pid = -1;
  for(int i=0; i<MAX_PROCESS_NUM; ++i)
    if(kernel->running[i] == 0) {
      pid = i;
      kernel->running[i] = 1;
      break;
    }
  if(pid == -1) // return -1 if full
    return -1;

  // alloc space for page_table
  kernel->mm[pid].page_table = (struct PageTable*)malloc(sizeof(struct PageTable));
  kernel->mm[pid].page_table->ptes = (struct PTE*)malloc(page * sizeof(struct PTE));
  kernel->mm[pid].size = size;

  // set all PFNs to -1 and present bytes to 0
  for(int i=0; i<page; ++i) {
    kernel->mm[pid].page_table->ptes[i].PFN = -1;
    kernel->mm[pid].page_table->ptes[i].present = 0;
  }

  // update global states
  kernel->allocated_pages += page;

  return pid;
}

/*
  This function will read the range [addr, addr+size) from user space of a specific process to the buf (buf should be >= size).
  1. Check if the reading range is out-of-bounds.
  2. If the pages in the range [addr, addr+size) of the user space of that process are not present,
     this leads to page fault.
     You should firstly map them to the free kernel-managed memory pages (first fit policy).
     You may use PFN, present, occupied_pages.
  3. Return 0 when success, -1 when failure (out of bounds).
  @param[in]  kernel    kernel simulator
  @param[in]  pid       process id
  @param[in]  addr      read content from [addr, addr+size)
  @param[in]  size      number of needed bytes
  @param[out] buf       buffer that should be filled with data
  @return               0 when success, -1 when failure
*/
int vm_read(struct Kernel* kernel, int pid, char* addr, int size, char* buf) {
  /* Fill Your Code Below */
  // retrieve the address value
  uintptr_t addr_value = (uintptr_t)(addr) / PAGE_SIZE;

  // calculate number of pages
  int page = size % PAGE_SIZE == 0 ?
             size / PAGE_SIZE : 
             size / PAGE_SIZE + 1;

  // check if out of bound
  if(addr_value < 0 || size > kernel->mm[pid].size)
    return -1;

  for(int buf_index = 0, first_free_page = 0;
      buf_index < page;
      ++addr_value, ++buf_index)
  {
    // map to the first free page in physical memory if it was not yet done so
    if(kernel->mm[pid].page_table->ptes[addr_value].present == 0) {
      for(int j=first_free_page; j<VIRTUAL_SPACE_SIZE/PAGE_SIZE; ++j)
        if(kernel->occupied_pages[j] == 0) {
          first_free_page = j;
          kernel->occupied_pages[j] = 1;
          break;
        }
      kernel->mm[pid].page_table->ptes[addr_value].PFN = first_free_page;
      kernel->mm[pid].page_table->ptes[addr_value].present = 1;
    }
    // copy data from buffer to physical memory
    int physical_addr = kernel->mm[pid].page_table->ptes[addr_value].PFN;
    memcpy(&buf[buf_index], &kernel->space[physical_addr], PAGE_SIZE);
  }
  return 0;
}

/*
  This function will write the content of buf to user space [addr, addr+size) (buf should be >= size).
  1. Check if the writing range is out-of-bounds.
  2. If the pages in the range [addr, addr+size) of the user space of that process are not present,
     this leads to page fault.
     You should firstly map them to the free kernel-managed memory pages (first fit policy).
     You may use PFN, present, occupied_pages.
  3. Return 0 when success, -1 when failure (out of bounds).
  @param[in]  kernel    kernel simulator
  @param[in]  pid       process id
  @param[in]  addr      read content from [addr, addr+size)
  @param[in]  size      number of needed bytes
  @param[in]  buf       data that should be written
  @return               0 when success, -1 when failure
*/
int vm_write(struct Kernel* kernel, int pid, char* addr, int size, char* buf) {
  /* Fill Your Code Below */
  // retrieve the address value
  uintptr_t addr_value = (uintptr_t)(addr) / PAGE_SIZE;

  // calculate number of pages
  int page = size % PAGE_SIZE == 0 ?
             size / PAGE_SIZE : 
             size / PAGE_SIZE + 1;

  // check if out of bound
  if(addr_value < 0 || size > kernel->mm[pid].size)
    return -1;

  for(int buf_index = 0, first_free_page = 0;
      buf_index < page;
      ++addr_value, ++buf_index)
  {
    // map to the first free page in physical memory if it was not yet done so
    if(kernel->mm[pid].page_table->ptes[addr_value].present == 0) {
      for(int j=first_free_page; j<VIRTUAL_SPACE_SIZE/PAGE_SIZE; ++j) {
        if(kernel->occupied_pages[j] == 0) {
          first_free_page = j;
          kernel->occupied_pages[j] = 1;
          break;
        }
      }
      kernel->mm[pid].page_table->ptes[addr_value].PFN = first_free_page;
      kernel->mm[pid].page_table->ptes[addr_value].present = 1;
    }
    // copy data from physical memory to buffer
    int physical_addr = kernel->mm[pid].page_table->ptes[addr_value].PFN;
    memcpy(&kernel->space[physical_addr], &buf[buf_index], PAGE_SIZE);
  }

  return 0;
}

/*
  This function will free the space of a process.
  1. Reset the corresponding pages in occupied_pages to 0, update allocated_pages
  2. Release the page_table in the corresponding MMStruct and set to NULL.
  3. Release the pid, reset running
  4. Return 0 when success, -1 when failure.
  @param[in]  kernel    kernel simulator
  @param[in]  pid       process id
  @return               0 when success, -1 when failure
*/
int proc_exit_vm(struct Kernel* kernel, int pid) {
  /* Fill Your Code Below */
  if(pid < 0 || pid >= MAX_PROCESS_NUM)
    return -1;

  // reset occupied_pages to 0
  for(int i=0; i<(kernel->mm[pid].size + PAGE_SIZE - 1) / PAGE_SIZE; ++i)
    if(kernel->mm[pid].page_table->ptes[i].present == 1)
      kernel->occupied_pages[
        kernel->mm[pid].page_table->ptes[i].PFN
                            ] = 0;

  kernel->allocated_pages -= (kernel->mm[pid].size + PAGE_SIZE - 1) / PAGE_SIZE;

  // delete page_table
  free(kernel->mm[pid].page_table->ptes);
  kernel->mm[pid].page_table->ptes = NULL;
  free(kernel->mm[pid].page_table);
  kernel->mm[pid].page_table = NULL;

  kernel->mm[pid].size = 0;

  kernel->running[pid] = 0;

  return 0;
}
