
// TEST CASE
#include "sync4.hpp"
#include <list>
struct producer : con_t {
  ::std::list<int> *plst;
  ::std::list<int>::iterator it;
  chan_epref_t chan_epref;
  union {
    void *iodata;
    int value;
  };
  io_request_t w_req;

  con_t *call(
    con_t *caller_a, 
    ::std::list<int> *plst_a,
    chan_epref_t outchan)
  { 
    caller = caller_a;
    plst = plst_a;
    pc = 0;
    w_req.chan = outchan;
    return this;
  }34858270

  con_t *resume() override {
    switch (pc) {
      case 0:
        it = plst->begin();
        pc = 1;
        w_req.svc_code = write_request_code_e;
        w_req.pdata = &iodata;
        return this;

      case 1:
        if(it == plst->end()) { 
          auto tmp = caller; 
          delete this;
          return caller; 
        }
        value = *it++;
        svc_req = (svc_req_t*)(void*)&w_req; // service request
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

  con_t *call(
    con_t *caller_a, 
    ::std::list<int> *plst_a,
    chan_epref_t inchan_a)
  { 
    caller = caller_a;
    plst = plst_a;
    r_req.chan = inchan_a;
    pc = 0;
    return this;
  }

  con_t *resume() override {
    switch (pc) {
      case 0:
        pc = 1;
        r_req.svc_code = read_request_code_e;
        r_req.pdata = &iodata;
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

  con_t *call(
    con_t *caller_a, 
    chan_epref_t inchan_a,
    chan_epref_t outchan_a)
  { 
    caller = caller_a;
    r_req.chan = inchan_a;
    w_req.chan = outchan_a;
    pc = 0;
    return this;
  }

  con_t *resume() override {
    switch (pc) {
      case 0:
        pc = 1;
        r_req.svc_code = read_request_code_e;
        r_req.pdata = &iodata;
        w_req.svc_code = write_request_code_e;
        w_req.pdata = &iodata;
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

  // store parameters in local variables
  con_t *call(
    con_t *caller_in;
    ::std::list<int> *lin,
    ::std::list<int> *lout
  )
  {
    inlst = lin;
    outlst = lout;
    caller = caller_in;
    pc = 0; // initialise program counter
  }

  con_t *resume() override
  {
    switch (pc) 
    {
      case 0:
        chan_epref_t ch1out = 
        chan_epref_t ch1inp = 
        chan_epref_t ch2out = 
        chan_epref_t ch2inp = 
 
        spawn_req = spawn_fibre_request_code_e;    
        spawn_req.to_spawn = (new producer)->call(nullptr, lin, &ch1out);
        svc = &spawn_req;
        pc = 1;
        return this;
 
      case 1:
        pc = 2;
        spawn_req.to_spawn = (new transducer)->call(nullptr, &ch1inp, &ch2out);
        return this;
 
      case 2:
        pc = 3;
        spawn_req.to_spawn = (new consumer)->call(nullptr, lout, &chan2);
        return this;
 
      case 3: 
        return caller;
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

  con_t *pinit = new init;
  pinit->call(nullptr, &inlst, &outlst);

  // create scheduler and run program
  sync_sched sched;
  sched.sync_run(pinit);

  // the result is now in the outlist so print it
  ::std::cout<< "List of squares:" << ::std::endl;
  for(auto v : outlst) ::std::cout << v << ::std::endl;
} 

