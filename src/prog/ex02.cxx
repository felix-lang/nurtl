
#include <iostream>
#include <cassert>
#include <list>
#include "csp.hpp"
#include "chips.hpp"

// producer
static int counter = 0;
int next() { ++counter; return counter; }
using next_t = int (*)();
using producer = chips::source<int,next_t>;

using bound = chips::bound<int>;

int square(int x) { return x * x; }
using square_t = int (*)(int);
using transducer = chips::transducer<int,int,square_t>;

void show(int x) { ::std::cout << x << ::std::endl; }
using show_t = void (*)(int);
using consumer = chips::sink<int, show_t>; 


#include "csp.hpp"
#include "chips.hpp"
struct init: con_t {
  ::std::list<int> *inlst;
  ::std::list<int> *outlst;
  spawn_fibre_request_t spawn_req;
  chan_epref_t ch1out;
  chan_epref_t ch1inp;
  chan_epref_t ch2out;
  chan_epref_t ch2inp;
  chan_epref_t ch3out;
  chan_epref_t ch3inp;

  init(fibre_t *f) : con_t(f) {}

  ~init() {}

  // store parameters in
  CSP_CALLDEF_START,
    ::std::list<int> *lin,
    ::std::list<int> *lout
  CSP_CALLDEF_MID
    inlst = lin;
    outlst = lout;
  CSP_CALLDEF_END

  CSP_RESUME_START
    ch1out = make_concurrent_channel(fibre->process->system->system_allocator);
    ch1inp = ch1out->dup(); 
    ch2out = make_concurrent_channel(fibre->process->system->system_allocator);
    ch2inp = ch2out->dup();
    ch3out = make_concurrent_channel(fibre->process->system->system_allocator);
    ch3inp = ch3out->dup();
 
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new(fibre->process->process_allocator) producer(nullptr))->call(nullptr, ch1out, next))
    SVC(&spawn_req)
 
  case 1:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new(fibre->process->process_allocator) bound(nullptr))->call(nullptr, ch1inp, ch2out, 20))
    SVC(&spawn_req)
 
  case 2:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new(fibre->process->process_allocator) transducer(nullptr))->call(nullptr, ch2inp, ch3out, square))
    SVC(&spawn_req)

  case 3:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new(fibre->process->process_allocator) consumer(nullptr))->call(nullptr, ch3inp, show))
    SVC(&spawn_req)

  case 4:
    CSP_RETURN

  CSP_RESUME_END
  CSP_SIZE
}; // init class

#include <iostream>

int main() {
  // create the input list
  ::std::list<int> inlst;
  for (auto i = 0; i < 20; ++i) inlst.push_back(i);

  // empty output list
  ::std::list<int> outlst;

  {
    ::std::vector<mem_req_t> reqs;
    reqs.push_back(mem_req_t {sizeof(producer),50});
    reqs.push_back(mem_req_t {sizeof(transducer),50});
    reqs.push_back(mem_req_t {sizeof(consumer),50});
    reqs.push_back(mem_req_t {sizeof(init),50});
    reqs.push_back(mem_req_t {256,50});
    reqs.push_back(mem_req_t {512,50});
 

    // bootstrap allocator
    alloc_ref_t malloc_free = new malloc_free_allocator_t; // parent C++ allocator
    alloc_ref_t malloc_free_debugger = new(malloc_free) debugging_allocator_t(malloc_free, malloc_free, "Malloc");


    // system allocator
    alloc_ref_t system_allocator_delegate = new(malloc_free_debugger) system_allocator_t(malloc_free_debugger,malloc_free_debugger, reqs);
    alloc_ref_t system_allocator_debugger = new(malloc_free_debugger) debugging_allocator_t(malloc_free_debugger, system_allocator_delegate, "Sys");
    alloc_ref_t system_allocator = new(malloc_free_debugger) statistics_allocator_t( malloc_free_debugger, system_allocator_debugger, "Sysalloc.stats.txt");


    // initial process will use the system allocator
    alloc_ref_t process_allocator = system_allocator;

    // creates the clock too
    system_t *system = new system_t(system_allocator);

    csp_run(system, process_allocator, (new(process_allocator) init(nullptr))-> call(nullptr, &inlst, &outlst));
::std::cerr << "RUN COMPLETE" << ::std::endl;
    delete system;
  }
} 

