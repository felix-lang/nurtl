
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <atomic>
#include <memory>

#include "con.hpp"
#include "fibre.hpp"
#include "channel.hpp"
#include "active_set.hpp"
#include "svc.hpp"
#include "sync_sched.hpp"

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
  switch(pc){\
  case 0:

#define CSP_RESUME_END\
  default: assert(false);\
  }}

#define CSP_CALL_DIRECT0(case_label,procedure)\
  pc=case_label;\
  return new procedure->call(this);\
  case case_label:;

#define CSP_CALL_DIRECT1(case_label,procedure,arg)\
  pc=case_label;\
  return new procedure->call(this,arg);\
  case case_label:;

#define CSP_CALL_DIRECT2(case_label,procedure,arg1,arg2)\
  pc=case_label;\
  return new procedure->call(this,arg1,arg2);\
  case case_label:;


#define CSP_CALL_DIRECT3(case_label,procedure,arg1,arg2,arg3)\
  pc=case_label;\
  return new procedure->call(this,arg1,arg2,arg3);\
  case case_label:;


