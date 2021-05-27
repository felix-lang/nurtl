struct allocator_t {
  virtual void *allocate(size_t)=0;
  virtual void deallocate(void *, size_t)=0;
  virtual ~allocator_t(){}
};

void *operator new(size_t amt, allocator_t& a) { return a.allocate (amt); } 
void destroy(void *object, size_t amt, allocator_t *a) { a->deallocate(object, amt); }

