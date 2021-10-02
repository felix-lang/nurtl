// continuation
struct con_t {
  fibre_t *fibre;
  int pc;        // program counter
  con_t () : pc(0), fibre(nullptr) {}
  virtual con_t *return_control()=0;
  virtual con_t *resume()=0;
  virtual size_t size()const=0;
  virtual ~con_t(){}
};

// coroutine
struct coroutine_t : con_t {
  con_t *return_control(); // deletes object and returns nullptr
};
 
// top level subroutine
struct subroutine_t : con_t {
  subroutine_t(fibre_t *f) { fibre = f; }
  con_t *caller; // caller continuation
  con_t *return_control(); // deletes object and returns caller
};

// NOTE: a subroutine must not delete itself if it returns
// a nested procedure which binds to its local variable frame

struct curry_subroutine_t : con_t {
  con_t *caller; // caller continuation
  curry_subroutine_t (fibre_t *f) { fibre = f; }

  con_t *return_control() {
    auto tmp = caller;
    caller = nullptr;
    return tmp;
  }
};
