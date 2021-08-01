#ifndef _CSP_SOURCE
#define _CSP_SOURCE
#include "csp.hpp"

namespace chips {
// source based on a C++ function C F()
// Note F must a C
// channel uses pointer to C for communication
// The data MUST be allocated by the process allocator
template<class C, class F>
struct source : con_t {
  chan_epref_t outchan;
  io_request_t w_req;
  C *outptr;
  F f;

  CSP_SIZE

  source(fibre_t *f) : con_t(f) {}
  ~source(){}

  CSP_CALLDEF_START,
    chan_epref_t outchan_a,
    F f_a
  CSP_CALLDEF_MID
    outchan = outchan_a;
    f = f_a;
  CSP_CALLDEF_END

  CSP_RESUME_START
    SVC_WRITE_REQ(&w_req,&outchan,&outptr);

  case 1:
    outptr = new(fibre->process->process_allocator) C(f());
    pc = 1;
    SVC(&w_req);

  CSP_RESUME_END
};

} // namespace

#endif

