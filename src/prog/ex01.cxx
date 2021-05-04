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
    ::std::cout << "Hello World" << ::std::endl;
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
  ~producer() { 
    // ::std::cout<< "producer "<<this<<" destructor" << ::std::endl; 
  }

  CSP_CALLDEF_START,
    ::std::list<int> *plst_a,
    chan_epref_t outchan
  CSP_CALLDEF_MID
    plst = plst_a;
    out = outchan;
  CSP_CALLDEF_END

  CSP_RESUME_START
    // ::std::cout << "Producer case 0" << ::std::endl;
    it = plst->begin();
    pc = 1;
    w_req.svc_code = write_request_code_e;
    w_req.pdata = &iodata;
    w_req.chan = &out;
    return this;

  case 1:
    // ::std::cout << "Producer case 1" << ::std::endl;
    if(it == plst->end()) { 
      // ::std::cout << "Producer finished" << ::std::endl;
      CSP_RETURN
    }
    value = *it++;
    // ::std::cout << "Producer writing .. " << ::std::endl;
    svc_req = (svc_req_t*)(void*)&w_req; // service request
    // ::std::cout << "Producer write complete.. " << ::std::endl;
    return this;
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

  ~consumer() { 
    // ::std::cout<< "consumer "<<this<<" destructor" << ::std::endl; 
  }

  CSP_CALLDEF_START,
    ::std::list<int> *plst_a,
    chan_epref_t inchan_a
  CSP_CALLDEF_MID
    plst = plst_a;
    inp = inchan_a;
  CSP_CALLDEF_END

  CSP_RESUME_START  
    // ::std::cout << "Consumer case 0" << ::std::endl;
    pc = 1;
    r_req.svc_code = read_request_code_e;
    r_req.pdata = &iodata;
    r_req.chan = &inp;
    return this;

  case 1:
    svc_req = (svc_req_t*)(void*)&r_req; // service request
    pc = 2;
    return this;

  case 2:
    plst->push_back(value);
    pc = 1;
    return this;
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

  ~transducer() { 
    // ::std::cout<< "transducer "<<this<<" destructor" << ::std::endl; 
  }

  CSP_CALLDEF_START,
    chan_epref_t inchan_a,
    chan_epref_t outchan_a
  CSP_CALLDEF_MID 
    inp = inchan_a;
    out = outchan_a;
  CSP_CALLDEF_END
  
  CSP_RESUME_START
    // ::std::cout << "Transducer case 0" << ::std::endl;
    pc = 1;
    r_req.svc_code = read_request_code_e;
    r_req.pdata = &iodata;
    r_req.chan = &inp;
    w_req.svc_code = write_request_code_e;
    w_req.pdata = &iodata;
    w_req.chan = &out;
    return this;

  case 1:
    svc_req = (svc_req_t*)(void*)&r_req; // service request
    pc = 2;
    return this;

  case 2:
    value = value * value; // square value
    svc_req = (svc_req_t*)(void*)&w_req; // service request
    pc = 1;
    return this;

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

  ~init() {
    // ::std::cout << "init " << this << " destructor" << ::std::endl;
    // ::std::cout << "ch1out refcnt = " << ch1out.use_count() << ::std::endl;
  }

  // store parameters in local variables
  CSP_CALLDEF_START,
    ::std::list<int> *lin,
    ::std::list<int> *lout
  CSP_CALLDEF_MID
    inlst = lin;
    outlst = lout;
  CSP_CALLDEF_END

  CSP_RESUME_START
    // ::std::cout << "init case 0" << ::std::endl;
   ch1out = make_channel();
    // ::std::cout << "After construction ch1out refcnt = " << ch1out.use_count() << ::std::endl;
    ch1inp = ch1out->dup(); 
    // ::std::cout << "After dup ch1out refcnt = " << ch1out.use_count() << ::std::endl;
    ch2out = make_channel();
    ch2inp = ch2out->dup();
 
    spawn_req.svc_code = spawn_fibre_request_code_e;    
    spawn_req.tospawn = (new producer)->call(nullptr, inlst, ch1out);
    ch1out.reset();
    // ::std::cout << "After move to producer fibre ch1out refcnt = " << ch1out.use_count() << ::std::endl;
    // ::std::cout << "After move to producer fibre ch1out pointer is = " << ch1out.get() << ::std::endl;
    // ::std::cout<< "Producer initialised" << ::std::endl;
    svc_req = (svc_req_t*)&spawn_req;
    pc = 1;
    // ::std::cout<< "Producer spawned" << ::std::endl;
    return this;
 
  case 1:
    // ::std::cout << "init case 1" << ::std::endl;
    pc = 2;
    spawn_req.tospawn = (new transducer)->call(nullptr, ch1inp, ch2out);
    ch1inp.reset();
    ch2out.reset();
    // ::std::cout << "After move ch1out refcnt = " << ch1out.use_count() << ::std::endl;
    svc_req = (svc_req_t*)&spawn_req;
    // ::std::cout<< "Transducer spawned" << ::std::endl;
    return this;
 
  case 2:
    // ::std::cout << "init case 2" << ::std::endl;
    pc = 3;
    spawn_req.tospawn = (new consumer)->call(nullptr, outlst,ch2inp);
    ch2inp.reset();
    svc_req = (svc_req_t*)&spawn_req;
    // ::std::cout<< "Consumer spawned" << ::std::endl;
    return this;
 
  case 3:
    pc = 4;
    spawn_req.tospawn = (new hello)->call(nullptr);
    svc_req = (svc_req_t*)&spawn_req;
    return this;

  case 4:
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

  init *pinit = new init;
  pinit->call(nullptr, &inlst, &outlst);

  // create scheduler and run program
  sync_sched sched(new active_set_t);
  sched.sync_run(pinit);

  // the result is now in the outlist so print it
  // ::std::cout<< "List of squares:" << ::std::endl;
  for(auto v : outlst) ::std::cout << v << ::std::endl;
} 

