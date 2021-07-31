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
    SVC(&w_req);
    pc = 1;

  CSP_RESUME_END
  size_t size() const override { return sizeof(*this); } 
};

} // namespace

#endif
}
#endif

