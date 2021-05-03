
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


