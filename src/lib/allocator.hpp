struct allocator_t {
  virtual void *allocate(size_t)=0;
  virtual void deallocate(void *, size_t)=0;
  virtual ~allocator_t()=0;
};
