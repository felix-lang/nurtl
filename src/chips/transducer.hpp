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
    SVC(&r_req);
    outptr = new(fibre->process->process_allocator) C(f(*inptr));
    delete_concrete_object(inptr, fibre->process->process_allocator);
    SVC(&w_req);
    pc = 1;

  CSP_RESUME_END
};

} // namespace

#endif

