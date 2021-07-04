// For use by the kernel CSP system
struct system_t {
  alloc_ref_t system_allocator;
  system_t (alloc_ref_t a) : system_allocator(a) {}
};

