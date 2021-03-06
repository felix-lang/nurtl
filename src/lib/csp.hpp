#ifndef _CSP
#define _CSP

// C++ resources
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <queue>
#include <iostream>
#include <map>
#include <utility>
#include <cstdlib>
#include <typeinfo>

// forward decls
struct csp_clock_t;
struct fibre_t;
struct channel_t;
struct csp_process_t;
struct con_t;
struct allocator_t;
struct alloc_ref_t;
struct system_t;
struct channel_endpoint_t;
struct chan_epref_t;

// the csp system
#include "allocator.hpp"
#include "malloc_free_allocator.hpp"
#include "utility_allocators.hpp"
#include "freelist.hpp"
#include "system_allocator.hpp"
#include "system.hpp"

#include "con.hpp"
#include "fibre.hpp"

#include "csp_process.hpp"
#include "svc.hpp"

con_t *coroutine_t::return_control()
{
//::std::cerr << "Coroutine " << this << " returns control" << ::std::endl;
  delete_csp_polymorphic_object(this,fibre->process->process_allocator);
  return nullptr;
}

con_t *subroutine_t::return_control() {
  auto tmp = caller;
  delete_csp_polymorphic_object(this,fibre->process->process_allocator);
  return tmp;
}


// resolve circular reference
fibre_t::~fibre_t()
{
  //::std::cerr << "Fibre destructor " << this << ::std::endl;
  while(cc) cc= cc->return_control(); 
}

#include "channel.hpp"
#include "csp_thread.hpp"
#include "sequential_channel.hpp"
#include "concurrent_channel.hpp"
#include "async_channel.hpp"
#include "clock.hpp"

#define CSP_SUBRETURN return return_control();
#define CSP_COSUICIDE return return_control();

#define CSP_CALLDEF_START \
  con_t *call(con_t *caller_a

#define CSP_CALLDEF_MID ){\
  caller = caller_a;\
  pc = 0;

#define CSP_CALLDEF_END \
  return this;\
}

#define CSP_CODEF_START \
  con_t *setup(

#define CSP_CODEF_MID ){\
  pc = 0;

#define CSP_CODEF_END \
  return this;\
}
#define CSP_RESUME_START\
  con_t *resume() override {\
  switch(pc++){\
  case 0:

#define CSP_RESUME_END\
  default: assert(false);\
  }}

#define CSP_SIZE \
  size_t size() const override { return sizeof(*this); } 

#define SVC_READ_REQ(xpreq,xpchan,xpdata)\
  (xpreq)->svc_code = read_request_code_e;\
  (xpreq)->pdata = (void**)xpdata;\
  (xpreq)->chan = xpchan;

#define SVC_WRITE_REQ(xpreq,xpchan,xpdata)\
  (xpreq)->svc_code = write_request_code_e;\
  (xpreq)->pdata = (void**)xpdata;\
  (xpreq)->chan = xpchan;

#define SVC_ASYNC_WRITE_REQ(xpreq,xpchan,xpdata)\
  (xpreq)->svc_code = async_write_request_code_e;\
  (xpreq)->pdata = (void**)xpdata;\
  (xpreq)->chan = xpchan;

#define SVC_SPAWN_FIBRE_REQ(xpreq,xcont)\
  (xpreq)->svc_code = spawn_fibre_request_code_e;\
  (xpreq)->tospawn = xcont;

#define SVC_SPAWN_FIBRE_DEFERRED_REQ(xpreq,xcont)\
  (xpreq)->svc_code = spawn_fibre_deferred_request_code_e;\
  (xpreq)->tospawn = xcont;

#define SVC_SPAWN_PTHREAD_REQ(xprec,xcont)\
  (xpreq)->spawn_pthread_request_code_e;\
  (xpreq)->tospawn = xcont;

#define SVC(preq)\
  fibre->svc_req = (svc_req_t*)(void*)preq;\
  return this;

#define CSP_GOTO(caseno)\
  pc = caseno;\
  return this;

/*
#define CSP_CALL_DIRECT0(procedure)\
  return (new procedure(global))->call(this);

#define CSP_CALL_DIRECT1(procedure,arg)\
  return (new procedure(global))->call(this,arg);

#define CSP_CALL_DIRECT2(procedure,arg1,arg2)\
  return (new procedure(global))->call(this,arg1,arg2);


#define CSP_CALL_DIRECT3(procedure,arg1,arg2,arg3)\
  return (new procedure(global))->call(this,arg1,arg2,arg3);
*/
#endif

