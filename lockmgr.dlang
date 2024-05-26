module klockmgr;


extern(C) struct spinlock;

extern(C) @system {
  void acquire(spinlock *);
  void release(spinlock *);
  int holding(spinlock *lk);
}

/* the API:
    struct [something] {
    // ...
    }
    spinlock *sl;
  */
struct KLockMgr(T) {
  private:
    T *struct_data;
    spinlock sl;
  this(T *struct_data) {
    this.struct_data = struct_data;
    acquire(&sl);
  }
  T *data()
  in (holding(sl)) {
    return struct_data;
  }
  ~this() {
    release(&sl);
  }
}
