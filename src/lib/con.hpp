// continuation
struct con_t {
  fibre_t *fibre;
  con_t (fibre_t *f) : fibre(f) {}
  con_t *caller; // caller continuation
  int pc;        // program counter
  virtual con_t *resume()=0;
  virtual size_t size()const=0;
  virtual ~con_t(){}
};


