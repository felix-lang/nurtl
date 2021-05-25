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

// client request entry: client needs n_blocks of size block_size
struct mem_req_t {
  size_t block_size;
  size_t n_blocks;
};

static size_t calblocksize(size_t n) {
  return n % 8 == 0 ? n : (n / 8 + 1) * 8;
}

struct wait_free_allocator_t {
  void **arena;                                   // the memory to be allocated
  wait_free_ring_buffer_t **ring_buffer_pointers; // array of ring buffer pointers 

  ~wait_free_allocator_t() { free(arena); }

  void *allocate(size_t n_bytes) { 
    return ring_buffer_pointers[calblocksize(n)]->pop(); 
  }
  void deallocate(void *memory, size_t bytes) { 
   ring_buffer_pointers[calblocksize(n)]->push(memory); 
  }

  // immobile
  wait_free_allocator_t(wait_free_allocator_t const&)=delete;
  wait_free_allocator_t &operator=(wait_free_allocator_t const&)=delete;

  wait_free_allocator(::std::vector<mem_req_t>);
};

// Allocator construction
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

// block sizes must be multiples of 8
// this may have to be changed to 16 if long double is used 
// and requires double word alignment
static size_t calringbuffersize(size_t v) {
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

void wait_free_allocator(::std::vector<mem_req_t> reqs) {

  // find the largest block requested
  size_t max_block_size = 0;
  for(auto req: reqs) maxblock = max(req.block_size, max_block_size);

  // make an array mapping block size to request count
  ::std::vector<size_t> nblocks;
  for (size_t i = 0; i <= max_block_size; ++i) 
    nblocks.push_back(0);
  for (auto req: reqs) 
    nblocks[calblocksize(req.block_size)] += req.n_blocks;

  // find user memory requirement
  size_t user_memory = 0;
  for (size_t i = 0; i <= max_block_size; ++i) 
    user_memory += i * nblocks[i]; 

  // calculate the store required for ring buffers
  size_t ring_buffer_memory = 0;
  for (size_t i = 0; i <= max_block_size; ++i) 
    ring_buffer_memory += calringbuffersize(nblocks[i]); 

  // calculate store required for ring buffer objects
  size_t ring_buffer_object_memory = 0
  for (size_t i = 0; i <= max_block_size; ++i) 
    ring_buffer_object_memory +=  (nblocks == 0 ? 0 : sizeof(wait_free_ring_buffer_t);

  // calculate store for ring_buffer_pointers
  size_t ring_buffer_pointer_memory = max_block_size * sizeof(ring_buffer_t*);

  // total store requirement
  size_t arena_memory = user_memory + ring_buffer_memory + ring_buffer_object_memory + ring_buffer_pointer_memory;

  arena = malloc(arena_memory);
 
}
 
