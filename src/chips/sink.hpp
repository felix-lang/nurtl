#ifndef _CSP_SINK
#define _CSP_SINK
#include "csp.hpp"

namespace chips {
// sink based on a C++ function C F()
// Note F must a C
// channel uses pointer to C for communication
// The data MUST be allocated by the process allocator
template<class C, class F>
struct sink;

template<class C, class F>
struct sink_arg2_t {
  sink_arg2_t (sink<C,F> *sink_a) : p(sink_a) {}
  sink<C,F> *p;
  sink<C,F> *operator()(chan_epref_t inchan_a) const;
};



template<class C, class F>
struct sink : con_t {
  chan_epref_t inchan;
  io_request_t r_req;
  C *inptr;
  F f;

  CSP_SIZE

  sink(fibre_t *f) : con_t(f) {}
  ~sink(){}

  sink_arg2_t<C,F> call( F f_a) {
    f = f_a;
    return sink_arg2_t<C,F>(this);
  }

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

template<class C, class F>
sink<C,F> *sink_arg2_t<C,F>::operator()(chan_epref_t inchan_a)const {
    p->inchan = inchan_a;
    return p;
}
}
#endif

