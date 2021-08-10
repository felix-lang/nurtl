// For use by the kernel CSP system
struct system_t {
  alloc_ref_t system_allocator;
  system_t (alloc_ref_t a) : system_allocator(a) {}

  void connect_sequential (chan_epref_t *left, chan_epref_t *right);
  void connect_concurrent (chan_epref_t *left, chan_epref_t *right);
  void connect_async(chan_epref_t *left, chan_epref_t *right);

};

