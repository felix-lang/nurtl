struct allocator_t {
  virtual void *allocate(size_t)=0;
  virtual void deallocate(void *, size_t)=0;
  virtual ~allocator_t(){};
};

void *operator new(size_t amt, allocator_t& a) { return a.allocate (amt); } 
void dealloc(void *object, size_t amt, allocator_t *a) { a->deallocate(object, amt); }

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
void delete_csp_polymorphic_object (T *object, allocator_t *a) { 
  size_t amt = object->size(); 
  object->~T();
  a->deallocate(object, amt); 
};


template <class T> 
struct allocator_template_t : ::std::allocator<T> {
  using value_type = T;
  allocator_t *allocator;
  allocator_template_t (allocator_t *a) : allocator(a) {}
  T *allocate(size_t amt) { return (T*)allocator->allocate(amt * sizeof(T)); }
  void deallocate(T *p, size_t amt) noexcept { return allocator->deallocate(p,amt * sizeof(T)); }
};

namespace std {
  template<class T> 
    struct allocator_traits<allocator_template_t<T>> {
    using is_always_equal = false_type; 
  };
}


