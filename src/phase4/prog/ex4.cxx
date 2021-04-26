#include <iostream>
#include <cassert>
// TEST CASE
#include "sync4.hpp"
#include <list>
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
    ::std::cout<< "producer "<<this<<" destructor" << ::std::endl; 
  }
  con_t *call(
    con_t *caller_a, 
    ::std::list<int> *plst_a,
    chan_epref_t &&outchan)
  { 
    caller = caller_a;
    plst = plst_a;
    pc = 0;
    out = outchan;
    return this;
  }

  con_t *resume() override {
    switch (pc) {
      case 0:
::std::cout << "Producer case 0" << ::std::endl;
        it = plst->begin();
        pc = 1;
        w_req.svc_code = write_request_code_e;
        w_req.pdata = &iodata;
        w_req.chan = &out;
        return this;

      case 1:
::std::cout << "Producer case 1" << ::std::endl;
        if(it == plst->end()) { 
::std::cout << "Producer finished" << ::std::endl;
          auto tmp = caller; 
          delete this;
          return tmp; 
        }
        value = *it++;
::std::cout << "Producer writing .. " << ::std::endl;
        svc_req = (svc_req_t*)(void*)&w_req; // service request
::std::cout << "Producer write complete.. " << ::std::endl;
        return this;
      default: assert(false);
    }
  }
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
    ::std::cout<< "consumer "<<this<<" destructor" << ::std::endl; 
  }

  con_t *call(
    con_t *caller_a, 
    ::std::list<int> *plst_a,
    chan_epref_t &&inchan_a)
  { 
    caller = caller_a;
    plst = plst_a;
    inp = inchan_a;
    pc = 0;
    return this;
  }

  con_t *resume() override {
    switch (pc) {
      case 0:
::std::cout << "Consumer case 0" << ::std::endl;
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
      default: assert(false);
    }
  }
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
    ::std::cout<< "transducer "<<this<<" destructor" << ::std::endl; 
  }

  con_t *call(
    con_t *caller_a, 
    chan_epref_t &&inchan_a,
    chan_epref_t &&outchan_a)
  { 
    caller = caller_a;
    inp = inchan_a;
    out = outchan_a;
    pc = 0;
    return this;
  }

  con_t *resume() override {
    switch (pc) {
      case 0:
::std::cout << "Transducer case 0" << ::std::endl;
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
      default: assert(false);
    }
  }
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
    ::std::cout << "init " << this << " destructor" << ::std::endl;
    ::std::cout << "ch1out refcnt = " << ch1out.use_count() << ::std::endl;
  }

  // store parameters in local variables
  con_t *call(
    con_t *caller_in,
    ::std::list<int> *lin,
    ::std::list<int> *lout
  )
  {
    inlst = lin;
    outlst = lout;
    caller = caller_in;
    pc = 0; // initialise program counter
    return this;
  }

  con_t *resume() override
  {
    switch (pc) 
    {
      case 0:
::std::cout << "init case 0" << ::std::endl;
        ch1out = make_channel();
        ch1inp = ch1out->dup(); 
        ch2out = make_channel();
        ch2inp = ch2out->dup();
 
        spawn_req.svc_code = spawn_fibre_request_code_e;    
        spawn_req.tospawn = (new producer)->call(nullptr, inlst, ::std::move(ch1out));
::std::cout<< "Producer initialised" << ::std::endl;
        svc_req = (svc_req_t*)&spawn_req;
        pc = 1;
::std::cout<< "Producer spawned" << ::std::endl;
        return this;
 
      case 1:
::std::cout << "init case 1" << ::std::endl;
        pc = 2;
        spawn_req.tospawn = (new transducer)->call(nullptr, ::std::move(ch1inp), ::std::move(ch2out));
        svc_req = (svc_req_t*)&spawn_req;
::std::cout<< "Transducer spawned" << ::std::endl;
        return this;
 
      case 2:
::std::cout << "init case 2" << ::std::endl;
        pc = 3;
        spawn_req.tospawn = (new consumer)->call(nullptr, outlst, ::std::move(ch2inp));
        svc_req = (svc_req_t*)&spawn_req;
::std::cout<< "Consumer spawned" << ::std::endl;
        return this;
 
      case 3: 
::std::cout << "init case 3" << ::std::endl;
        {
::std::cout << "Init finished" << ::std::endl;
          con_t *tmp = caller;
          delete this;
          return tmp;
        }

      default: assert(false);
    } // switch
  } // resume
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
  ::std::cout<< "List of squares:" << ::std::endl;
  for(auto v : outlst) ::std::cout << v << ::std::endl;
} 

