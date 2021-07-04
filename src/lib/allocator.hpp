struct allocator_t {
  ::std::atomic<size_t> refcnt;
  allocator_t() : refcnt(1) {}
  virtual void *allocate(size_t)=0;
  virtual void deallocate(void *, size_t)=0;
  virtual ~allocator_t(){
    ::std::cerr << "Allocator deleted" << ::std::endl; 
  }
  virtual size_t size()const=0;
};

// smart pointer to allocators
struct alloc_ref_t {
  allocator_t *allocator;
  alloc_ref_t(allocator_t *p) : allocator(p) {}  // allocator ctor sets refcnt to 1 initially
  alloc_ref_t() : allocator(nullptr) {}          // default null

  allocator_t *operator->()const { return allocator; }
  allocator_t *get() const { return allocator; }

  // rvalue move
  alloc_ref_t(alloc_ref_t &&p) {
    if(p.allocator) { 
      allocator = p.allocator; 
      p.allocator = nullptr; 
    }
    else allocator = nullptr;
  } 

  // lvalue copy
  alloc_ref_t(alloc_ref_t &p);

  // rvalue assign
  void operator= (alloc_ref_t &&p) {
    if (&p!=this) { // ignore self assign
      this->~alloc_ref_t(); // destroy target
      new(this) alloc_ref_t(::std::move(p)); // move source to target
      p.allocator = nullptr;
    }
  } // rass

  // lvalue assign
  void operator= (alloc_ref_t &p) { 
    if(&p!=this) { // ignore self assign
      this->~alloc_ref_t(); // destroy target
      new(this) alloc_ref_t(p); // copy source to target
    }
  } // lass
 
  // destructor
  // uses allocator passed to allocator on construction to delete it
  ~alloc_ref_t();
};

// lvalue copy 
alloc_ref_t::alloc_ref_t(alloc_ref_t &p) {
  if(p.allocator) ++p.allocator->refcnt;
  allocator = p.allocator;
}

// destructor
alloc_ref_t::~alloc_ref_t() {
  if(allocator) {
    if(allocator->refcnt == 1) delete allocator; // FIXME
    else --allocator->refcnt;
  }
}

// user access
void *operator new(size_t amt, allocator_t& a) { return a.allocate (amt); } 
void *operator new(size_t amt, alloc_ref_t& a) { assert(a.get()); return a->allocate (amt); } 
//void dealloc(void *object, size_t amt, allocator_t *a) { a->deallocate(object, amt); }

// NOTE: will not work on pointers to base classes
// To do this correctly for polymorphic objects
// would require C++20 deleting destructors, and even then,
// It can NEVER work wth a pointer to a base of non-polymorphic class
// 
// We propose to add a size() virtual method to polymorphic objects
// to fix this, but this will ONLY work on our own types, not foreign ones.
//
template<class T>
void delete_concrete_object (T *object, allocator_t *a) { 
  object->~T(); a->deallocate(object, sizeof(T)); 
};

template<class T>
void delete_concrete_object (T *object, alloc_ref_t a) { 
  delete_concrete_object(object, a.get()); 
};

template<class T>
void delete_csp_polymorphic_object (T *object, allocator_t *a) { 
  size_t amt = object->size(); 
  object->~T();
  a->deallocate(object, amt); 
};

template<class T>
void delete_csp_polymorphic_object (T *object, alloc_ref_t a) { 
  delete_csp_polymorphic_object(object,a.get());
};


