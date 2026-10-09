struct buf {
  int valid;   // has data been read from disk? b->data contains an up-to-date copy of the block on disk(no disk read needed on subsequent bread)
  int disk;    // does disk "own" buf? If disk controller currently has an active DmA request on b->data. kernel threads must wait.
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};
