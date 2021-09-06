#include <iostream>
#include <cassert>
#include <list>


#include "csp.hpp"
#include "chips.hpp"

// TEST CASE

struct hello : coroutine_t {

  CSP_RESUME_START
    ::std::cerr << "Hello World" << ::std::endl;
    CSP_COSUICIDE
  CSP_RESUME_END
  CSP_SIZE
};


struct producer : coroutine_t {
  ::std::list<int> *plst;
  ::std::list<int>::iterator it;
  chan_epref_t out;
  union {
    void *iodata;
    int value;
  };
  io_request_t w_req;

  ~producer() { }

  CSP_CODEF_START
    ::std::list<int> *plst_a,
    chan_epref_t outchan_a
  CSP_CODEF_MID
    plst = plst_a;
    out = outchan_a;
  CSP_CODEF_END

  CSP_RESUME_START
    it = plst->begin();
    SVC_WRITE_REQ(&w_req,&out,&iodata);

  case 1:
    if(it == plst->end()) { 
      CSP_COSUICIDE
    }
    value = *it++;
    pc = 1;
    SVC(&w_req)

  CSP_RESUME_END
  CSP_SIZE
};

struct consumer: coroutine_t {
  ::std::list<int> *plst;
  union {
    void *iodata;
    int value;
  };
  io_request_t r_req;
  chan_epref_t inp;

  ~consumer() {}

  CSP_CODEF_START
    ::std::list<int> *plst_a,
    chan_epref_t inchan_a
  CSP_CODEF_MID
    plst = plst_a;
    inp = inchan_a;
  CSP_CODEF_END

  CSP_RESUME_START  
    SVC_READ_REQ(&r_req,&inp,&iodata)

  case 1:
    SVC(&r_req)

  case 2:
    ::std::cerr << "Consumer gets " << value << ::std::endl;
    plst->push_back(value);
    CSP_GOTO(1)

  CSP_RESUME_END
  CSP_SIZE
};

struct square : subroutine_t {
  int inp;
  int *pout;

  square(fibre_t *f) : subroutine_t(f) {}
 
  CSP_CALLDEF_START,
    int *pout_a,
    int inp_a
  CSP_CALLDEF_MID
    pout = pout_a;
    inp = inp_a;
  CSP_CALLDEF_END

  CSP_RESUME_START
    *pout = inp * inp;
    {
      con_t *tmp = caller;
      delete_concrete_object(this, fibre->process->process_allocator);
      return tmp;
    }
  CSP_RESUME_END
  CSP_SIZE
};


struct transducer: coroutine_t {
  union {
    void *iodata;
    int value;
  };
  io_request_t r_req;
  io_request_t w_req;
  chan_epref_t inp;
  chan_epref_t out;

  ~transducer() {}

  CSP_CODEF_START
    chan_epref_t inchan_a,
    chan_epref_t outchan_a
  CSP_CODEF_MID 
    inp = inchan_a;
    out = outchan_a;
  CSP_CODEF_END
  
  CSP_RESUME_START
    SVC_READ_REQ(&r_req,&inp,&iodata)
    SVC_WRITE_REQ(&w_req,&out,&iodata)

  case 1:
    SVC(&r_req)

  case 2:
    //CSP_CALL_DIRECT2(square,&value,value)
    return (new(fibre->process->process_allocator) square(fibre))->call(this,&value,value);

  case 3:
    pc = 1;
    SVC(&w_req)

  CSP_RESUME_END
  CSP_SIZE
};


struct init: coroutine_t {
  ::std::list<int> *inlst;
  ::std::list<int> *outlst;
  spawn_fibre_request_t spawn_req;
  chan_epref_t ch1out;
  chan_epref_t ch1inp;
  chan_epref_t ch2out;
  chan_epref_t ch2inp;
  chan_epref_t clock_connection;
  io_request_t clock_req;
  double waituntil;
  double *pwaituntil;
  ::std::shared_ptr<csp_clock_t> clock;

  ~init() {}

  // store parameters in
  CSP_CODEF_START
    ::std::list<int> *lin,
    ::std::list<int> *lout
  CSP_CODEF_MID
    inlst = lin;
    outlst = lout;
  CSP_CODEF_END

  CSP_RESUME_START
    ch1out = make_concurrent_channel(fibre->process->system->system_allocator);
    ch1inp = ch1out->dup(); 
    ch2out = make_concurrent_channel(fibre->process->system->system_allocator);
    ch2inp = ch2out->dup();
 
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new(fibre->process->process_allocator) producer)->setup(inlst, ch1out))
    SVC(&spawn_req)
 
  case 1:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new(fibre->process->process_allocator) transducer)->setup(ch1inp, ch2out))
    SVC(&spawn_req)
 
  case 2:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new(fibre->process->process_allocator) consumer)->setup(outlst,ch2inp))
    SVC(&spawn_req)
 
  case 3:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new(fibre->process->process_allocator) hello))
    SVC(&spawn_req)

  case 4:
    { 
      clock = make_clock(fibre->process->system);
      clock->start();
      //::std::cerr << "Clock started, time is " << clock->now() << ::std::endl;
      clock_connection = clock->connect();
      //::std::cerr << "Got connection" << ::std::endl;
      {
        double rightnow = clock->now();
        waituntil = rightnow + 12.10;
        //::std::cerr << ::std::fixed << "Wait until" << waituntil << " for " << waituntil - rightnow << " seconds" << ::std::endl;
      }
      //::std::cerr << "Alarm time " << waituntil << ", stored at " << &waituntil
      //  << "=" << pwaituntil << " which is stored at " << &pwaituntil << ::std::endl;
      pwaituntil = &waituntil;
      SVC_WRITE_REQ(&clock_req,&clock_connection,&pwaituntil);
      ::std::cerr<<"****** INIT Sleeping ********" << ::std::endl;
    }
// too early to signal
// need different kind of channel w. cv
    SVC(&clock_req)
// too late to signal
  case 5:
    // if this doesn't print, we didn't resume after the sleep correctly
    ::std::cerr<<"****** INIT Sleep Over ********" << ::std::endl;
    CSP_COSUICIDE


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
    reqs.push_back(mem_req_t {sizeof(hello),50});
    reqs.push_back(mem_req_t {sizeof(producer),50});
    reqs.push_back(mem_req_t {sizeof(transducer),50});
    reqs.push_back(mem_req_t {sizeof(consumer),50});
    reqs.push_back(mem_req_t {sizeof(init),50});
    reqs.push_back(mem_req_t {sizeof(square),50});
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

    csp_run(system, process_allocator, (new(process_allocator) init)->setup(&inlst, &outlst));
::std::cerr << "RUN COMPLETE" << ::std::endl;
    delete system;
  }

  // the result is now in the outlist so print it
  ::std::cerr<< "main: +++++++++ List of squares:" << ::std::endl;
  for(auto v : outlst) ::std::cerr << v << ::std::endl;
  ::std::cerr<< "main: +++++++++ Done" << ::std::endl;
} 

