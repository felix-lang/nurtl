// continuation
struct con_t {
  global_t *global;
  fibre_t *fibre;
  con_t (global_t *g) : global(g), fibre(nullptr) {}
  con_t *caller; // caller continuation
  int pc;        // program counter
  virtual con_t *resume()=0;
  virtual ~con_t(){}
};


