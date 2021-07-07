#include <iomanip>

// client request entry: client needs n_blocks of size block_size
struct mem_req_t {
  size_t block_size;
  size_t n_blocks;
};

// given a memory block size of n bytes,
// calculate the smallest legitimate allocation
// amount which can contain n bytes
//
// currently we just round up to a multiple of 8
static size_t calblocksize(size_t n) {
  return n % 8 == 0 ? n : (n / 8 + 1) * 8;
}

struct system_allocator_t : allocator_t {
  alloc_ref_t delegate;
  unsigned char *arena;  // the memory to be allocated 
  size_t arena_size;

  freelist_t **freelist_pointers; // array of ring buffer pointers 

  size_t size()const override { return sizeof(*this); }
  ~system_allocator_t() override { delegate->deallocate(arena, arena_size); }

  void *allocate(size_t n_bytes) override { 
    auto n = calblocksize(n_bytes);
    auto rb = freelist_pointers[n]; 
    //::std::cerr << "freelist pointer arrray = " << freelist_pointers << ::std::endl;
    //std::cerr << "zzz alloc " << n_bytes << ", n:" << n << " rb: " << rb << "\n";
    //rb->log();
    auto p = rb->pop();
    //std::cerr << "zzz p: " << p << "\n";
    return p;

//    return freelist_pointers[calblocksize(n_bytes)]->pop(); 
  }
  void deallocate(void *memory, size_t bytes) override { 
    freelist_pointers[calblocksize(bytes)]->push(memory); 
  }

  // immobile
  system_allocator_t(system_allocator_t const&)=delete;
  system_allocator_t &operator=(system_allocator_t const&)=delete;

  system_allocator_t(alloc_ref_t, alloc_ref_t, ::std::vector<mem_req_t>);
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


system_allocator_t::system_allocator_t(alloc_ref_t p, alloc_ref_t d, ::std::vector<mem_req_t> reqs) : allocator_t(p), delegate(d) {

  ::std::cerr << ::std::setbase(16);

  // find the largest block requested
  size_t max_block_size = 0;
  for(auto req: reqs) max_block_size = ::std::max(req.block_size, max_block_size);
//  ::std::cerr << "max_block_size " << max_block_size << ::std::endl;

  // make an array mapping block size to request count
  ::std::vector<size_t> nblocks;
  for (size_t i = 0; i <= max_block_size; ++i) 
    nblocks.push_back(0);
  for (auto req: reqs) 
    nblocks[calblocksize(req.block_size)] += req.n_blocks;
  
//  ::std::cerr << "nblocks array: " << ::std::endl;
//  for(size_t k = 0; k<= max_block_size; ++k)
//    if(nblocks[k])
//      ::std::cerr << "  " << k  << ": " << nblocks[k] << ::std::endl;

  // find user memory requirement
  size_t user_memory = 0;
  for (size_t i = 0; i <= max_block_size; ++i) 
    user_memory += i * nblocks[i]; 
//  ::std::cerr << "user_memory: " << user_memory << ::std::endl;

  // calculate the store required for freelists 
  size_t freelist_memory = 0;
  for (size_t i = 0; i <= max_block_size; ++i) 
    freelist_memory += nblocks[i] * sizeof(void*); 
//  ::std::cerr << "freelist_memory: " << freelist_memory << ::std::endl;

  // calculate store required for freelist objects
  size_t freelist_object_memory = 0;
  for (size_t i = 0; i <= max_block_size; ++i) 
    freelist_object_memory +=  (nblocks[i] == 0 ? 0 : sizeof(freelist_t));
//  ::std::cerr << "freelist_object_memory: " << freelist_object_memory << ::std::endl;

  // calculate store for freelist_pointers
  size_t freelist_pointer_memory = (max_block_size + 1) * sizeof(freelist_t*);
//  ::std::cerr << "freelist_pointer_memory: " << freelist_object_memory << ::std::endl;

  // total store requirement
  arena_size = user_memory + freelist_memory + freelist_object_memory + freelist_pointer_memory;
  ::std::cerr << "arena_size: " << arena_size<< ::std::endl;

  arena = (unsigned char*)(delegate->allocate (arena_size));
  ::std::cerr << "arena: " << (void*)arena << ::std::endl;

  unsigned char *arena_pointer = arena;

  // allocate ring buffer pointer array 
  freelist_pointers = (freelist_t**)(void*)arena_pointer; 
//  ::std::cerr << "freelist_pointers: " << (void*)freelist_pointers<< ::std::endl;

  arena_pointer += freelist_pointer_memory;

  // allocate and initialise freelist objects
  freelist_t *freelist_object_pointer;
  for(size_t k = max_block_size; k>0; --k) {
    if(nblocks[k] == 0) {
      freelist_pointers[k] = freelist_object_pointer;
//      std::cerr << "freelist_object["<<k<<"]: " << (void*)freelist_object_pointer<< ::std::endl;
    }
    else {
      // pointer to ring buffer object
      freelist_object_pointer = (freelist_t*)(void*)arena_pointer;
      freelist_pointers[k] = freelist_object_pointer;
//      ::std::cerr << "freelist_object["<<k<<"]: " << (void*)freelist_object_pointer<< ::std::endl;
      arena_pointer += sizeof(freelist_t);

      // pointer to actual freelist 
      void **freelist_stack_top = (void**)(void*)arena_pointer;
//      ::std::cerr << "freelist_stack_top "<<k<<": " << (void*)freelist_stack_top<< ::std::endl;
      size_t n_entries = nblocks[k];
      arena_pointer += n_entries * sizeof(void*);

      // initialise freelist object
      new(freelist_object_pointer) freelist_t (n_entries, freelist_stack_top);

      // allocate the client storage
      for(size_t i = 0; i < nblocks[k]; ++i) {
        freelist_object_pointer->push(arena_pointer); // one block
        arena_pointer += k;
      } 
    }
  }
  assert(arena_pointer == arena + arena_size); 
}
 
