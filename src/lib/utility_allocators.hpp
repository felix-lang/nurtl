struct debugging_allocator_t : allocator_t {
  alloc_ref_t delegate;
  char const *tag;
  debugging_allocator_t(char const *tg, alloc_ref_t p, alloc_ref_t a) : tag(tg), allocator_t(p), delegate(a) {}
  ~debugging_allocator_t() override {}
  virtual size_t size()const override { return sizeof(*this); }

  void *allocate(size_t n) override { 
    auto p = delegate->allocate(n);
    ::std::cerr << "  " << tag << "++Alloc   " << p << "[" << n << "]" << ::std::endl;
    return p;
  }
  void deallocate(void *p, size_t n) override { 
    ::std::cerr << "  " << tag << "--Dealloc " << p << "[" << n << "]" << ::std::endl;
    delegate->deallocate(p,n);
  }
};

