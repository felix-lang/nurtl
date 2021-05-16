
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

#include "con.hpp"
#include "fibre.hpp"
#include "svc.hpp"
#include "channel.hpp"
#include "active_set.hpp"
#include "sync_sched.hpp"
#include "sequential_channel.hpp"
#include "concurrent_channel.hpp"
#include "async_channel.hpp"
#include "clock.hpp"

#define CSP_RETURN {\
  con_t *tmp = caller;\
  delete this;\
  return tmp;\
}

#define CSP_CALLDEF_START \
  con_t *call(con_t *caller_a

#define CSP_CALLDEF_MID ){\
  caller = caller_a;\
  pc = 0;

#define CSP_CALLDEF_END \
  return this;\
}

#define CSP_RESUME_START\
  con_t *resume() override {\
  switch(pc++){\
  case 0:

#define CSP_RESUME_END\
  default: assert(false);\
  }}


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
  svc_req = (svc_req_t*)(void*)preq;\
  return this;

#define CSP_GOTO(caseno)\
  pc = caseno;\
  return this;

#define CSP_CALL_DIRECT0(procedure)\
  return (new procedure)->call(this);

#define CSP_CALL_DIRECT1(procedure,arg)\
  return (new procedure)->call(this,arg);

#define CSP_CALL_DIRECT2(procedure,arg1,arg2)\
  return (new procedure)->call(this,arg1,arg2);


#define CSP_CALL_DIRECT3(procedure,arg1,arg2,arg3)\
  return (new procedure)->call(this,arg1,arg2,arg3);


