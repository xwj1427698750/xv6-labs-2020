// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
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

#define BUCKET_NUM 13

struct {
    struct spinlock lock;
    struct buf buf[NBUF];
    struct spinlock bucket_locks[BUCKET_NUM];
    // Linked list of all buffers, through prev/next.
    // Sorted by how recently the buffer was used.
    // head.next is most recent, head.prev is least.
    struct buf buckets[BUCKET_NUM];
} bcache;

void
binit(void) {
    struct buf *b;

    for (int i = 0; i < BUCKET_NUM; i++) {
        initlock(&bcache.bucket_locks[i], "bcache.bucket");
        bcache.buckets[i].prev = &bcache.buckets[i];
        bcache.buckets[i].next = &bcache.buckets[i];
    }
  // Create linked list of buffers

    for(int i = 0; i < NBUF; i++ ){
        b = &bcache.buf[i];
        int hash = i % BUCKET_NUM;
        hash = 0;
        b->next = bcache.buckets[hash].next;
        b->prev = &bcache.buckets[hash];
        initsleeplock(&b->lock, "buffer");
        bcache.buckets[hash].next->prev = b;
        bcache.buckets[hash].next = b;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno) {
    struct buf *b;
    int hash = blockno % BUCKET_NUM;
    acquire(&bcache.bucket_locks[hash]);

    // Is the block already cached?
    for (b = bcache.buckets[hash].next; b != &bcache.buckets[hash]; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release(&bcache.bucket_locks[hash]);
            acquiresleep(&b->lock);
            return b;
        }
    }

    // Not cached.
    // Recycle the least recently used (LRU) unused buffer.
    // 在当前的bucket里搜寻最久没有使用的缓存块
    for (b = bcache.buckets[hash].prev; b != &bcache.buckets[hash]; b = b->prev) {
        if (b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            release(&bcache.bucket_locks[hash]);
            acquiresleep(&b->lock);
            return b;
        }
    }
    //从其他的bucket里偷一块, //释放掉自己，由小锁换成大的锁，防止bucket A 等待 bucket B的锁， bucket B 等待 bucket A的锁
    release(&bcache.bucket_locks[hash]);
    acquire(&bcache.lock);
    for (int i = 0; i < BUCKET_NUM; i++) {
        if (i == hash) continue;
        acquire(&bcache.bucket_locks[i]);
        for (b = bcache.buckets[i].prev; b != &bcache.buckets[i]; b = b->prev) {
            if (b->refcnt == 0) {
                b->dev = dev;
                b->blockno = blockno;
                b->valid = 0;
                b->refcnt = 1;
                /***断掉节点与原先的bucket之间的联系*/
                b->next->prev = b->prev;
                b->prev->next = b->next;
                release(&bcache.bucket_locks[i]);
                /**将新的缓存块移动到新的bucket*/
                acquire(&bcache.bucket_locks[hash]);

                b->next = bcache.buckets[hash].next;
                b->prev = &bcache.buckets[hash];
                bcache.buckets[hash].next->prev = b;
                bcache.buckets[hash].next = b;

                release(&bcache.bucket_locks[hash]);

                release(&bcache.lock);
                acquiresleep(&b->lock);
                return b;
            }
        }
        release(&bcache.bucket_locks[i]);
    }
    panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno) {
    struct buf *b;

    b = bget(dev, blockno);
    if (!b->valid) {
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b) {
    if (!holdingsleep(&b->lock))
        panic("bwrite");
    virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b) {
    if (!holdingsleep(&b->lock))
        panic("brelse");

    releasesleep(&b->lock);
    int hash = b->blockno % BUCKET_NUM;
    acquire(&bcache.bucket_locks[hash]);
    b->refcnt--;
    if (b->refcnt == 0) {
        // no one is waiting for it.
        b->next->prev = b->prev;//断掉节点之间的联系
        b->prev->next = b->next;
        b->next = bcache.buckets[hash].next;
        b->prev = &bcache.buckets[hash];
        bcache.buckets[hash].next->prev = b;
        bcache.buckets[hash].next = b;
    }
    release(&bcache.bucket_locks[hash]);
}

void
bpin(struct buf *b) {
    acquire(&bcache.lock);
    b->refcnt++;
    release(&bcache.lock);
}

void
bunpin(struct buf *b) {
    acquire(&bcache.lock);
    b->refcnt--;
    release(&bcache.lock);
}


