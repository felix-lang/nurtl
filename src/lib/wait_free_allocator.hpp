#include <iomanip>

// client request entry: client needs n_blocks of size block_size
struct mem_req_t {
  size_t block_size;
  size_t n_blocks;
};

// given a memory block size of n bytes,
// calculate the smallest legitimate allocation
// amount which can contain n bytes

static size_t calblocksize(size_t n) {
  return n % 8 == 0 ? n : (n / 8 + 1) * 8;
}

struct wait_free_allocator_t : allocator_t {
  allocator_t *allocator;
  unsigned char *arena;  // the memory to be allocated 
  size_t arena_size;

  wait_free_ring_buffer_t **ring_buffer_pointers; // array of ring buffer pointers 

  ~wait_free_allocator_t() override { allocator->deallocate(arena, arena_size); }

  void *allocate(size_t n_bytes) override { 
auto rb = ring_buffer_pointers[calblocksize(n_bytes)]; 
std::cerr << "zzz alloc " << n_bytes << " rb: " << rb << "\n";
rb->log();
auto p = rb->pop();
std::cerr << "zzz p: " << p << "\n";
return p;

    return ring_buffer_pointers[calblocksize(n_bytes)]->pop(); 
  }
  void deallocate(void *memory, size_t bytes) override { 
    ring_buffer_pointers[calblocksize(bytes)]->push(memory); 
  }

  // immobile
  wait_free_allocator_t(wait_free_allocator_t const&)=delete;
  wait_free_allocator_t &operator=(wait_free_allocator_t const&)=delete;

  wait_free_allocator_t(allocator_t *, ::std::vector<mem_req_t>);
};

// ***************************************************************************
// Allocator construction
// ***************************************************************************
//
// The allocator construction is messy and difficult; we have to get
// the maths exactly right.
//
// We will use an array of pointers to ring buffers, where a request for
// a particular word size will automatically map to the smallest available
// block size greater than or equal to the request without any searching.
//
// The ring buffers themselves can be for ANY word size, however they
// must have a power of 2 slots. However they're initialised only with the
// maximum number of blocks needed of that size. The power of two is 
// required so that the head and tail indices continue to be correct
// after wrapping around past 2^64.
//
// We allocate EVERYTHING in the arena so that the allocator constructor
// and destructor only require a single malloc() and free(). This is important
// to improve startup and termination times (because ALL the memory ever
// needed is allocate on startup and remains allocated until the allocator
// is destroyed).
// ***************************************************************************


// Given the number of blocks v of some size required
// round up to the smallest power of 2 containing v entries
// 
// this is required so the modular arithmetic continues
// to be correct when the indices roll past 2^64
//
// Thus ring buffers may contain up to twice minus one
// wasted slots.
// and requires double word alignment
//
// Note: if we have one operation per pico-second
// the counter will roll over in 213 days
// If we have one operation per nano-second
// the counter will roll over after 584 years
//
// thus for most practical purposes, this routine
// could be replaced by the identity function,
// however the memory savings are likely small
// unless there are a lot of very small blocks
//
static size_t calringbuffernentries(size_t v) {
 v--;
 v |= v >> 1;
 v |= v >> 2;
 v |= v >> 4;
 v |= v >> 8;
 v |= v >> 16;
 v |= v >> 32;
 v++;
 return v;
}

wait_free_allocator_t::wait_free_allocator_t(allocator_t *a, ::std::vector<mem_req_t> reqs) : allocator(a) {

  ::std::cerr << ::std::setbase(16);

  // find the largest block requested
  size_t max_block_size = 0;
  for(auto req: reqs) max_block_size = ::std::max(req.block_size, max_block_size);
  ::std::cerr << "max_block_size " << max_block_size << ::std::endl;

  // make an array mapping block size to request count
  ::std::vector<size_t> nblocks;
  for (size_t i = 0; i <= max_block_size; ++i) 
    nblocks.push_back(0);
  for (auto req: reqs) 
    nblocks[calblocksize(req.block_size)] += req.n_blocks;
  
  ::std::cerr << "nblocks array: " << ::std::endl;
  for(size_t k = 0; k<= max_block_size; ++k)
    ::std::cerr << "  " << k  << ": " << nblocks[k] << ::std::endl;

  // find user memory requirement
  size_t user_memory = 0;
  for (size_t i = 0; i <= max_block_size; ++i) 
    user_memory += i * nblocks[i]; 
  ::std::cerr << "user_memory: " << user_memory << ::std::endl;

  // calculate the store required for ring buffers
  size_t ring_buffer_memory = 0;
  for (size_t i = 0; i <= max_block_size; ++i) 
    ring_buffer_memory += calringbuffernentries(nblocks[i]) * sizeof(void*); 
  ::std::cerr << "ring_buffer_memory: " << ring_buffer_memory << ::std::endl;

  // calculate store required for ring buffer objects
  size_t ring_buffer_object_memory = 0;
  for (size_t i = 0; i <= max_block_size; ++i) 
    ring_buffer_object_memory +=  (nblocks[i] == 0 ? 0 : sizeof(wait_free_ring_buffer_t));
  ::std::cerr << "ring_buffer_object_memory: " << ring_buffer_object_memory << ::std::endl;

  // calculate store for ring_buffer_pointers
  size_t ring_buffer_pointer_memory = (max_block_size + 1) * sizeof(wait_free_ring_buffer_t*);
  ::std::cerr << "ring_buffer_pointer_memory: " << ring_buffer_object_memory << ::std::endl;

  // total store requirement
  arena_size = user_memory + ring_buffer_memory + ring_buffer_object_memory + ring_buffer_pointer_memory;
  ::std::cerr << "arena_size: " << arena_size<< ::std::endl;

  arena = (unsigned char*)(allocator->allocate (arena_size));
  ::std::cerr << "arena: " << (void*)arena << ::std::endl;

  unsigned char *arena_pointer = arena;

  // allocate ring buffer pointer array 
  ring_buffer_pointers = (wait_free_ring_buffer_t**)(void*)arena_pointer; 
  ::std::cerr << "ring_buffer_pointers: " << (void*)ring_buffer_pointers<< ::std::endl;

  arena_pointer += ring_buffer_pointer_memory;

  // allocate and initialise ring buffer objects
  wait_free_ring_buffer_t *ring_buffer_object_pointer;
  for(size_t k = max_block_size; k>0; --k) {
    if(nblocks[k] == 0) ring_buffer_pointers[k] = ring_buffer_object_pointer;
    else {
      // pointer to ring buffer object
      wait_free_ring_buffer_t *ring_buffer_object_pointer = (wait_free_ring_buffer_t*)(void*)arena_pointer;
      ring_buffer_pointers[k] = ring_buffer_object_pointer;
      ::std::cerr << "ring_buffer_object["<<k<<"]: " << (void*)ring_buffer_object_pointer<< ::std::endl;
      arena_pointer += sizeof(wait_free_ring_buffer_t);

      // pointer to actual ring buffer
      void **ring_buffer_buffer_pointer = (void**)(void*)arena_pointer;
      ::std::cerr << "ring_buffer_buffer_pointer "<<k<<": " << (void*)ring_buffer_buffer_pointer<< ::std::endl;
      size_t n_entries = calringbuffernentries(nblocks[k]);
      arena_pointer += n_entries * sizeof(void*);

      // initialise ring buffer object
      new(ring_buffer_object_pointer) wait_free_ring_buffer_t (n_entries, ring_buffer_buffer_pointer);

      // allocate the client storage
      for(size_t i = 0; i < nblocks[k]; ++i) {
        ring_buffer_object_pointer->push(arena_pointer); // one block
        arena_pointer += k;
      } 
    }
  }
  assert(arena_pointer == arena + arena_size); 
}
 
