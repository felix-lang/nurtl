// fibre3.hpp
// fibre
struct fibre_t {
  con_t *cc;
  fibre_t *next;
  struct active_set_t *owner;
  union svc_req_t *svc_req; // request

  // default DEAD
  //fibre_t() : cc(nullptr), next(nullptr), owner(nullptr) {}

  // construct from continuation
  fibre_t(con_t *ccin, struct active_set_t *owned_by) : cc(ccin), next (nullptr), owner(owned_by), svc_req(nullptr) {}

  // immobile
  fibre_t(fibre_t const&)=delete;
  fibre_t& operator=(fibre_t const&)=delete;

  // destructor deletes any remaining continuations in spaghetti stack
  ~fibre_t() {
    while(cc) {
      con_t *tmp = cc->caller;
      delete cc;
      cc = tmp;
    }
  }
 
  // run until either fibre issues a service request or dies
  svc_req_t *run_fibre() { 
    while(cc) {
      cc=cc->resume(); 
      if(cc && svc_req) return svc_req;
    }
    return nullptr;
  }
};


