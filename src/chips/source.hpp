#ifndef _CSP_SOURCE
#define _CSP_SOURCE
#include "csp.hpp"

namespace chips {
// source based on a C++ function C F()
// Note F must a C
// channel uses pointer to C for communication
// The data MUST be allocated by the process allocator

template<class C, class F>
struct source;

template<class C, class F>
struct source_arg2_t {
  source_arg2_t (source<C,F> *source_a) : p(source_a) {}
  source<C,F> *p;
  source<C,F> *operator()(chan_epref_t outchan_a) const;
};

template<class C, class F>
struct source : con_t {
  chan_epref_t outchan;
  io_request_t w_req;
  C *outptr;
  F f;

  CSP_SIZE

  source(fibre_t *f) : con_t(f) {}
  ~source(){}

  source_arg2_t<C,F> call( F f_a) {
    f = f_a;
    return source_arg2_t<C,F>(this);
  }

  source *setup(F f_a) { f = f_a; return this; }

  CSP_RESUME_START
    SVC_WRITE_REQ(&w_req,&outchan,&outptr);

  case 1:
    outptr = new(fibre->process->process_allocator) C(f());
    pc = 1;
    SVC(&w_req);

  CSP_RESUME_END
};

template<class C, class F>
source<C,F> *source_arg2_t<C,F>::operator()(chan_epref_t outchan_a)const {
    p->outchan = outchan_a;
    return p;
}

} // namespace

#endif

