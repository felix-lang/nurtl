// channel3.hpp

// low bit fiddling routines
inline static bool get_lowbit(void *p) { 
  return (uintptr_t)p & (uintptr_t)1u; 
}
inline static void *clear_lowbit(void *p) { 
  return (void*)((uintptr_t)p & ~(uintptr_t)1u); 
}
inline static void *set_lowbit(void *p) { 
  return (void*)((uintptr_t)p | (uintptr_t)1u); 
}

// channel
struct channel_t {
  fibre_t *top;
  
  channel_t () : top (nullptr) {}

  // destructor deletes all continuations left in channel
  ~channel_t() {
    while (top) {
      fibre_t *f = (fibre_t*)clear_lowbit(top);
      fibre_t *tmp = f->next;
      delete f;
      top = tmp;
    }
  }

  // push a fibre as a reader: precondition it must be a reader
  // and, if the channel is non-empty it must contain only readers
  void push_reader(fibre_t *r) { 
    r->next = top; 
    top = r; 
  }

  // push a fibre as a writer: precondition it must be a writer
  // and, if the channel is non-empty it must contain only writers
  void push_writer(fibre_t *w) { 
    w->next = top; 
    top = (fibre_t*)set_lowbit(w);
  }

  // pop a reader if there is one, otherwise nullptr
  fibre_t *pop_reader() { 
    fibre_t *tmp = top; 
    if(get_lowbit(tmp))return nullptr;
    top = top -> next;
    return tmp; // lowbit is clear, its a reader 
  }

  // pop a writer if there is one, otherwise nullptr
  fibre_t *pop_writer() { 
    fibre_t *tmp = top; 
    if(!get_lowbit(tmp)) return nullptr;
    tmp = (fibre_t*)clear_lowbit(tmp); // lowbit is set for writer
    top = tmp -> next;
    return tmp;
  }
};

