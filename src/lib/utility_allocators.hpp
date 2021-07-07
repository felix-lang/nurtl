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

struct statistics_allocator_t : allocator_t {
  alloc_ref_t delegate;

  char const *filename;
  using stat_t = ::std::map<size_t, ::std::pair<size_t,size_t> >;
  using stat_rec_t = stat_t::value_type;
  stat_t stats;

  statistics_allocator_t(char const *tg, alloc_ref_t p, alloc_ref_t a) : filename(tg), allocator_t(p), delegate(a) {}
  virtual size_t size()const override { return sizeof(*this); }

  void *allocate(size_t n) override { 
    auto p = delegate->allocate(n);
    auto loc = stats.find(n);
    if(loc == stats.end()) {
      stat_rec_t rec = {n, {1, 1}};
      stats.insert(rec);
    } 
    else {
      auto rec = *loc;
      rec.second.first++;
      if(rec.second.first> rec.second.second) rec.second.second = rec.second.first; // max allocated
      stats[n] = rec.second;
    }
    return p;
  }
  void deallocate(void *p, size_t n) override { 
    delegate->deallocate(p,n);
    auto loc = stats.find(n);
    if(loc == stats.end()) {
      ::std::cerr << "Deallocate block of size " << n << " but that size block has never been allocated" << ::std::endl;
      //::std::abort();
      return;
    }
    auto rec = *loc;
    rec.second.first--;
    if(rec.second.first> rec.second.second) rec.second.second = rec.second.first; // max allocated
    stats[n] = rec.second;
  }

  ~statistics_allocator_t() override {
    auto outfile = fopen(filename,"w");
    ::std::cerr << "Stats written to " << filename << ::std::endl;
    for (auto rec : stats)
      fprintf(outfile, "%8lu: %8lu\n",rec.first, rec.second.second);
    fclose(outfile);
  }
};

