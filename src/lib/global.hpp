struct global_t {
  csp_clock_t *system_clock;
  allocator_t *system_allocator;
  system_allocator_t *real_time_allocator;
  global_t() {
    system_allocator = new malloc_free_allocator_t;
    system_clock = new csp_clock_t(system_allocator);
  }
};
