// For use by the kernel CSP system
struct system_t {
  allocator_t *system_allocator;
  system_t (allocator_t *a) : system_allocator(a) {}
};

