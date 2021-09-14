// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk bhashlock
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk bhashlock used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  struct spinlock lock;
  struct spinlock hashlock[NBUCKET];
  struct buf buf[NBUF];
  struct buf head[NBUCKET];
  uint global_tstamp;
} bcache;

void
binit(void)
{
  struct buf *b;

  int i;
  initlock(&bcache.lock, "bcache");
  for(i=0; i<NBUCKET; i++){
    initlock(&bcache.hashlock[i], "bcache");
    // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int idx = blockno % NBUCKET;
  acquire(&bcache.hashlock[idx]);

  // Is the block already cached?
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.hashlock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // to prevent from multiple cores operating on the same buffer
  release(&bcache.hashlock[idx]); // to prevent deadlock
  acquire(&bcache.lock);
  acquire(&bcache.hashlock[idx]);

  // recheck, since there may be other proc load the buffer during the gap
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock); // and also release bcache.lock
      release(&bcache.hashlock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  while(1){
    uint min_tstamp = 0xffffffff;
    struct buf* evict_buf = 0;
    for(b = bcache.buf; b != bcache.buf+NBUF; b++){
      if(b->refcnt == 0 && min_tstamp > b->tstamp) {
        evict_buf = b;
        min_tstamp = b->tstamp;
      }
    }
    int old_idx = evict_buf->blockno % NBUCKET;
    
    int f = 0;
    if(evict_buf->prev){ // if in a bucket
      if(old_idx != idx){ // if not the same bucket
        f = 1;
        acquire(&bcache.hashlock[old_idx]);
        if(evict_buf->refcnt != 0){ // there mey be other proc operating on it
          release(&bcache.hashlock[old_idx]);
          continue;
        }
      }
      evict_buf->prev->next = evict_buf->next;
    }
    // replace the LRU buffer
    evict_buf->dev = dev;
    evict_buf->blockno = blockno;
    evict_buf->valid = 0;
    evict_buf->refcnt = 1;
    if(evict_buf->next){
      evict_buf->next->prev = evict_buf->prev;
    }
    evict_buf->next = bcache.head[idx].next;
    evict_buf->prev = &bcache.head[idx];
    if(bcache.head[idx].next){
      bcache.head[idx].next->prev = evict_buf;
    }
    bcache.head[idx].next = evict_buf;
    if(f){
      release(&bcache.hashlock[old_idx]);
    }
    release(&bcache.lock);
    release(&bcache.hashlock[idx]);
    acquiresleep(&evict_buf->lock);
    return evict_buf;
  }
  
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  b->tstamp = ++bcache.global_tstamp;
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
  b->tstamp = ++bcache.global_tstamp;
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int idx = b->blockno % NBUCKET;
  acquire(&bcache.hashlock[idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->tstamp = 0;
    // no one is waiting for it.
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bcache.head[idx].next;
    // b->prev = &bcache.head[idx];
    // bcache.head[idx].next->prev = b;
    // bcache.head[idx].next = b;
  }
  
  release(&bcache.hashlock[idx]);
}

void
bpin(struct buf *b) {
  int idx = b->blockno % NBUCKET;
  acquire(&bcache.hashlock[idx]);
  b->refcnt++;
  b->tstamp = ++bcache.global_tstamp;
  release(&bcache.hashlock[idx]);
}

void
bunpin(struct buf *b) {
  int idx = b->blockno % NBUCKET;
  acquire(&bcache.hashlock[idx]);
  b->refcnt--;
  release(&bcache.hashlock[idx]);
}


