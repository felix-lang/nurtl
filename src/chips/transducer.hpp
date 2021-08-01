#ifndef _CSP_TRANSDUCER
#define _CSP_TRANSDUCER

#include "csp.hpp"

namespace chips {
// transducer based on a C++ function C F(D)
// Note F must accept a D and return a C
// channels used pointers to D and C for communication
// The data MUST be allocated by the process allocator
template<class D, class C, class F>
struct transducer : con_t {
  chan_epref_t inchan;
  chan_epref_t outchan;
  io_request_t r_req;
  io_request_t w_req;
  D *inptr;
  C *outptr;
  F f;

  CSP_SIZE

  transducer(fibre_t *f) : con_t(f) {}
  ~transducer(){}

  CSP_CALLDEF_START,
    chan_epref_t inchan_a,
    chan_epref_t outchan_a,
    F f_a
  CSP_CALLDEF_MID
    inchan = inchan_a;
    outchan = outchan_a;
    f = f_a;
  CSP_CALLDEF_END

  CSP_RESUME_START
    SVC_READ_REQ(&r_req,&inchan,&inptr);
    SVC_WRITE_REQ(&w_req,&outchan,&outptr);

  case 1:
    pc = 2;
    SVC(&r_req);

  case 2:
    outptr = new(fibre->process->process_allocator) C(f(*inptr));
    delete_concrete_object(inptr, fibre->process->process_allocator);
    pc = 1;
    SVC(&w_req);

  CSP_RESUME_END
};

template<class T>
struct bound : con_t {
  chan_epref_t inchan;
  chan_epref_t outchan;
  io_request_t r_req;
  io_request_t w_req;
  T *ptr;
  size_t counter;

  CSP_SIZE

  bound(fibre_t *f) : con_t(f) {}
  ~bound(){}

  CSP_CALLDEF_START,
    chan_epref_t inchan_a,
    chan_epref_t outchan_a,
    size_t counter_a 
  CSP_CALLDEF_MID
    inchan = inchan_a;
    outchan = outchan_a;
    counter = counter_a;
  CSP_CALLDEF_END

  CSP_RESUME_START
    SVC_READ_REQ(&r_req,&inchan,&ptr);
    SVC_WRITE_REQ(&w_req,&outchan,&ptr);

  case 1:
    if (counter == 0) goto finish;
    pc = 2;
    SVC(&r_req);

  case 2:
    --counter;
    pc = 1;
    SVC(&w_req);

  finish:
    CSP_RETURN

  CSP_RESUME_END
};


} // namespace

#endif

