#ifndef _CSP_SINK
#define _CSP_SINK
#include "csp.hpp"

namespace chips {
// sink based on a C++ function C F()
// Note F must a C
// channel uses pointer to C for communication
// The data MUST be allocated by the process allocator
template<class C, class F>
struct sink : con_t {
  chan_epref_t inchan;
  io_request_t r_req;
  C *inptr;
  F f;

  CSP_SIZE

  sink(fibre_t *f) : con_t(f) {}
  ~sink(){}

  CSP_CALLDEF_START,
    chan_epref_t inchan_a,
    F f_a
  CSP_CALLDEF_MID
    inchan = inchan_a;
    f = f_a;
  CSP_CALLDEF_END

  CSP_RESUME_START
    SVC_READ_REQ(&r_req,&inchan,&inptr);

  case 1:
  again:
    pc = 2;
    SVC(&r_req);

  case 2:
    f(*inptr);
    delete_concrete_object(inptr, fibre->process->process_allocator);
    goto again;

  CSP_RESUME_END
};
}
#endif

