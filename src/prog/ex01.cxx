#include <iostream>
#include <cassert>
// TEST CASE
#include "sync.hpp"
#include <list>

struct hello : con_t {
  CSP_CALLDEF_START
  CSP_CALLDEF_MID
  CSP_CALLDEF_END
  CSP_RESUME_START
    ::std::cerr << "Hello World" << ::std::endl;
    CSP_RETURN
  CSP_RESUME_END
};


struct producer : con_t {
  ::std::list<int> *plst;
  ::std::list<int>::iterator it;
  chan_epref_t out;
  union {
    void *iodata;
    int value;
  };
  io_request_t w_req;
  ~producer() { }

  CSP_CALLDEF_START,
    ::std::list<int> *plst_a,
    chan_epref_t outchan_a
  CSP_CALLDEF_MID
    plst = plst_a;
    out = outchan_a;
  CSP_CALLDEF_END

  CSP_RESUME_START
    it = plst->begin();
    SVC_WRITE_REQ(&w_req,&out,&iodata);

  case 1:
    if(it == plst->end()) { 
      CSP_RETURN
    }
    value = *it++;
    pc = 1;
    SVC(&w_req)

  CSP_RESUME_END
};

struct consumer: con_t {
  ::std::list<int> *plst;
  union {
    void *iodata;
    int value;
  };
  io_request_t r_req;
  chan_epref_t inp;

  ~consumer() {}

  CSP_CALLDEF_START,
    ::std::list<int> *plst_a,
    chan_epref_t inchan_a
  CSP_CALLDEF_MID
    plst = plst_a;
    inp = inchan_a;
  CSP_CALLDEF_END

  CSP_RESUME_START  
    SVC_READ_REQ(&r_req,&inp,&iodata)

  case 1:
    SVC(&r_req)

  case 2:
    plst->push_back(value);
    CSP_GOTO(1)

  CSP_RESUME_END
};

struct square : con_t {
  int inp;
  int *pout;
 
  CSP_CALLDEF_START,
    int *pout_a,
    int inp_a
  CSP_CALLDEF_MID
    pout = pout_a;
    inp = inp_a;
  CSP_CALLDEF_END

  CSP_RESUME_START
    *pout = inp * inp;
    CSP_RETURN
  CSP_RESUME_END
};


struct transducer: con_t {
  union {
    void *iodata;
    int value;
  };
  io_request_t r_req;
  io_request_t w_req;
  chan_epref_t inp;
  chan_epref_t out;

  ~transducer() {}

  CSP_CALLDEF_START,
    chan_epref_t inchan_a,
    chan_epref_t outchan_a
  CSP_CALLDEF_MID 
    inp = inchan_a;
    out = outchan_a;
  CSP_CALLDEF_END
  
  CSP_RESUME_START
    SVC_READ_REQ(&r_req,&inp,&iodata)
    SVC_WRITE_REQ(&w_req,&out,&iodata)

  case 1:
    SVC(&r_req)

  case 2:
    CSP_CALL_DIRECT2(square,&value,value)

  case 3:
    pc = 1;
    SVC(&w_req)

  CSP_RESUME_END
};


struct init: con_t {
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
  CSP_CALLDEF_START,
    ::std::list<int> *lin,
    ::std::list<int> *lout
  CSP_CALLDEF_MID
    inlst = lin;
    outlst = lout;
  CSP_CALLDEF_END

  CSP_RESUME_START
    ch1out = make_channel();
    ch1inp = ch1out->dup(); 
    ch2out = make_channel();
    ch2inp = ch2out->dup();
 
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new producer)->call(nullptr, inlst, ch1out))
    SVC(&spawn_req)
 
  case 1:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new transducer)->call(nullptr, ch1inp, ch2out))
    SVC(&spawn_req)
 
  case 2:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new consumer)->call(nullptr, outlst,ch2inp))
    SVC(&spawn_req)
 
  case 3:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new hello)->call(nullptr))
    SVC(&spawn_req)

  case 4:
    clock = make_clock();
    ::std::cerr << "Clock started, time is " << clock->now() << ::std::endl;
    clock_connection = clock->connect();
    ::std::cerr << "Got connection" << ::std::endl;
    {
      double rightnow = clock->now();
      waituntil = rightnow + 12.10;
      ::std::cerr << ::std::fixed << "Wait until" << waituntil << " for " << waituntil - rightnow << " seconds" << ::std::endl;
    }
    ::std::cerr << "Alarm time " << waituntil << ", stored at " << &waituntil
      << "=" << pwaituntil << " which is stored at " << &pwaituntil << ::std::endl;
    pwaituntil = &waituntil;
    SVC_ASYNC_WRITE_REQ(&clock_req,&clock_connection,&pwaituntil);
    SVC(&clock_req)

  case 5:
    ::std::cerr<<"Finished" << ::std::endl;
    CSP_RETURN 


  CSP_RESUME_END
}; // init class

#include <iostream>

int main() {
  // create the input list
  ::std::list<int> inlst;
  for (auto i = 0; i < 20; ++i) inlst.push_back(i);

  // empty output list
  ::std::list<int> outlst;

  csp_run((new init)-> call(nullptr, &inlst, &outlst));

  // the result is now in the outlist so print it
  // ::std::cerr<< "List of squares:" << ::std::endl;
  for(auto v : outlst) ::std::cerr << v << ::std::endl;
} 

