// For use by the kernel CSP system
struct system_t {
  allocator_t *allocator;
  system_t (allocator_t *a) : allocator(a) {}
};

