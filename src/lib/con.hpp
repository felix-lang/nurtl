// continuation
struct con_t {
  global_t *global;
  con_t (global_t *g) : global(g) {}
  con_t *caller; // caller continuation
  int pc;        // program counter
  union svc_req_t *svc_req; // request
  virtual con_t *resume()=0;
  virtual ~con_t(){}
};


