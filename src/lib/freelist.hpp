// the stack grows from high memory to low
// the base of the stack is one past the end

struct freelist_t {
  void **top;
  void **base;
  ::std::atomic_flag lock;

  freelist_t(size_t n, void **p) : top(p+n), base(p+n) {}

  void push(void *p) {
    while(lock.test_and_set(::std::memory_order_acquire)); // spin
    --top;
    *top = p;
    lock.clear(::std::memory_order_release); // release lock
  }

  void *pop() {
    while(lock.test_and_set(::std::memory_order_acquire)); // spin
    auto result = *top; 
    ++top;
    lock.clear(::std::memory_order_release); // release lock
    return result;
  }

  void log() const {
    for(auto p = base - 1; p <= top; --p) ::std::cerr << *p << ", ";
    ::std::cerr << ::std::endl;
  }
};

