struct VMA {
  uint64 addr, length;
  int offset;
  int prot;
  int flags;
  struct file* f;
  int busy;
};
