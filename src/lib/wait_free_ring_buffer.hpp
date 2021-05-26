  
// wait free ring buffer
// Semantics: 
// Pushing beyond capacity is undefined behaviour
// Popping when empty is undefined behaviour
// More than 2^64 pushes is undefined behaviour unless the capacity is a power of 2
//
// Implementation: 
// head points at next slot for a push modulo buffer size
// tail points at next slot for a pop modulo buffer size
// both operations use post increment
//
// Performance: wait free and blindingly fast
//
struct wait_free_ring_buffer_t {
  ::std::atomic<size_t> head;
  ::std::atomic<size_t> tail; // invariant head >= tail
  size_t const n_entries;     // should be power of 2
  void **data;

  // client must allocate buffer with malloc() 
  // and grants us exclusive ownership
  // the buffer can be either empty or full (but not partially filled)
  wait_free_ring_buffer_t (size_t n, void **d) : n_entries(n), head(0), tail(0), data(d) {}

  // uncopyable
  void wait_free_ring_buffer_t(wait_free_ring_buffer_t const&) = delete;
  wait_free_ring_buffer_t &operator=(wait_free_ring_buffer_t const&) = delete;

  // destructor
  ~wait_free_ring_buffer_t() { free(data); }

  void push(void *entry) { data[head++ % n_entries] = entry; }
  void *pop() { return data[head++ % n_entries]; }
};


