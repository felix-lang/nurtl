#ifndef _CSP_TRANSDUCER
#define _CSP_TRANSDUCER

#include "csp.hpp"

namespace chips {
// transducer based on a C++ function C F(D)
// Note F must accept a D and return a C
// channels used pointers to D and C for communication
// The data MUST be allocated by the process allocator
template<class D, class C, class F>
struct transducer;

template<class D, class C, class F>
struct transducer_arg2_t {
  transducer_arg2_t (transducer<D,C,F> *transducer_a) : p(transducer_a) {}
  transducer<D,C,F> *p;
  transducer<D,C,F> *operator()(chan_epref_t inchan_a, chan_epref_t outchan_a) const;
};


template<class D, class C, class F>
struct transducer : coroutine_t {
  chan_epref_t inchan;
  chan_epref_t outchan;
  io_request_t r_req;
  io_request_t w_req;
  D *inptr;
  C *outptr;
  F f;

  CSP_SIZE

  ~transducer(){}

  transducer_arg2_t<D,C,F> call( F f_a) {
    f = f_a;
    return transducer_arg2_t<D,C,F>(this);
  }

  transducer *setup(F f_a) { f = f_a; return this; }

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

template<class D, class C, class F>
transducer<D,C,F> *transducer_arg2_t<D,C,F>::operator()(chan_epref_t inchan_a, chan_epref_t outchan_a)const {
  p->inchan = inchan_a;
  p->outchan = outchan_a;
  return p;
}

// ------------------------------------------------------
// Bound
// A transducer which copies inp9ut to output a fixed number of times then suicides
template<class T>
struct bound;

template<class T>
struct bound_arg2_t {
  bound_arg2_t (bound<T> *bound_a) : p(bound_a) {}
  bound<T> *p;
  bound<T> *operator()(chan_epref_t inchan_a, chan_epref_t outchan_a) const;
};

template<class T>
struct bound : coroutine_t {
  chan_epref_t inchan;
  chan_epref_t outchan;
  io_request_t r_req;
  io_request_t w_req;
  T *ptr;
  size_t counter;

  CSP_SIZE

  ~bound(){}

  bound_arg2_t<T> call(size_t counter_a) {
    counter = counter_a;
    return bound_arg2_t<T>(this);
  }

  bound *setup(size_t counter_a) { counter = counter_a; return this; }


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
    CSP_COSUICIDE

  CSP_RESUME_END
};

template<class T>
bound<T> *bound_arg2_t<T>::operator()(chan_epref_t inchan_a, chan_epref_t outchan_a)const {
    p->inchan = inchan_a;
    p->outchan = outchan_a;
    return p;
}



} // namespace

#endif

