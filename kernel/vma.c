#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "vma.h"

struct {
  struct spinlock lock;
  struct VMA vma;
} vma_table[NOFILE]; // since kernel don't have its memory alloc mechanism

void 
vma_init(void){
  int i;
  for(i = 0; i < NOFILE; i++){
    initlock(&vma_table[i].lock, "vma");
  }
}

struct VMA*
alloc_vma(void){
  int i;
  for(i = 0; i < NOFILE; i++){
    acquire(&vma_table[i].lock);
    if(vma_table[i].vma.busy == 0){
      release(&vma_table[i].lock);
      vma_table[i].vma.busy = 1;
      return &vma_table[i].vma;
    }
    release(&vma_table[i].lock);
  }
  return 0;
}

void 
free_vma(struct VMA* vma){
  vma->busy = 0;
}