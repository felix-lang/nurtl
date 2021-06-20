
struct malloc_free_allocator_t : allocator_t {
  void *allocate(size_t n) override { 
    auto p = malloc(n); 
// ::std::cerr << "malloc(" << n << ") = " << p << ::std::endl; 
    return p; 
  }
  void deallocate(void *p, size_t n) override { free(p); }
  ~malloc_free_allocator_t() override {}
};


