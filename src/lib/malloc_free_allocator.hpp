
struct malloc_free_allocator_t : allocator_t {
  void *allocate(size_t n) override { return malloc(n); }
  void deallocate(void *p, size_t n) override { free(p); }
};


