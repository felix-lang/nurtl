// should be called tracing allocator
// prints every allocation and deallocation
// 
struct debugging_allocator_t : allocator_t {
  alloc_ref_t delegate;
  char const *tag;
  debugging_allocator_t(alloc_ref_t p, alloc_ref_t a, char const *tg) : tag(tg), allocator_t(p), delegate(a) {}
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

// TODO: checks 
// every allocation is deallocated
// every deallocation was previously allocated

struct checking_allocator_t : allocator_t {
};

// saves statistics to a file
// stats saved: max allocation at any one time for each block size
// could do more eg: 
// * total allocations for each block size
// * max memory allocated at any one time
//
struct statistics_allocator_t : allocator_t {
  alloc_ref_t delegate;

  char const *filename;
  using stat_t = ::std::map<size_t, ::std::pair<size_t,size_t> >;
  using stat_rec_t = stat_t::value_type;
  stat_t stats;

  statistics_allocator_t(alloc_ref_t p, alloc_ref_t a, char const *tg ) : filename(tg), allocator_t(p), delegate(a) {}
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

// TODO: wraps a spinlock around every allocate and deallocate
struct spinlocking_allocator_t : allocator_t {
};

// TODO: wraps a mutex around every allocate and deallocate
struct mutexing_allocator_t : allocator_t {
};

