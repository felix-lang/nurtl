// con3.hpp
// continuation
struct con_t {
  con_t *caller; // caller continuation
  int pc;        // program counter
  union svc_req_t *svc_req; // request
  virtual con_t *resume()=0;
  virtual ~con_t(){}
};


