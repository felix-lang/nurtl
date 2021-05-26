struct global_t {
  ::std::unique_ptr<csp_clock_t> system_clock;
  ::std::unique_ptr<allocator_t> system_allocator;
  wait_free_allocator_t *real_time_allocator;
  global_t() : system_clock(new csp_clock_t), system_allocator(new malloc_free_allocator_t), real_time_allocator(nullptr) {}
};

