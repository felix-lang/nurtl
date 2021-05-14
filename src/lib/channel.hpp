// CHANNEL ABSTRACTION

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
  ::std::atomic_size_t refcnt;
  fibre_t *top;
  
  channel_t () : top (nullptr) {}

  // immobile object
  channel_t(channel_t const&)=delete;
  channel_t& operator= (channel_t const&)=delete;

  // push a fibre as a reader: precondition it must be a reader
  // and, if the channel is non-empty it must contain only readers
  virtual void push_reader(fibre_t *r)=0;

  // push a fibre as a writer: precondition it must be a writer
  // and, if the channel is non-empty it must contain only writers
  virtual void push_writer(fibre_t *w)=0;

  // pop a reader if there is one, otherwise nullptr
  virtual fibre_t *pop_reader()=0;

  // pop a writer if there is one, otherwise nullptr
  virtual fibre_t *pop_writer()=0;

  virtual void signal()=0;

  // channel read operation
  virtual read(void **target, fibre_t **pcurrent)=0;

  // channel write operation
  virtual write(void **source, fibre_t **pcurrent)=0;

protected:
  // basic push and pop operations, not thread safe

  void st_push_reader(fibre_t *r) { 
    r->next = top; 
    top = r; 
  }

  void st_push_writer(fibre_t *w) { 
    w->next = top; 
    top = (fibre_t*)set_lowbit(w);
  }

  fibre_t *st_pop_reader() { 
    fibre_t *tmp = top; 
    if(!tmp || get_lowbit(tmp))return nullptr;
    top = top -> next;
    return tmp; // lowbit is clear, its a reader 
  }

  fibre_t *st_pop_writer() { 
    fibre_t *tmp = top; 
    if(!tmp || !get_lowbit(tmp)) return nullptr;
    tmp = (fibre_t*)clear_lowbit(tmp); // lowbit is set for writer
    top = tmp -> next;
    return tmp;
  }

};

struct channel_endpoint_t {
  channel_t *channel;
  channel_endpoint_t(channel_t *p) : channel(p) { ++channel->refcnt; }

  // immobile object
  channel_endpoint_t(channel_endpoint_t const&)=delete;
  channel_endpoint_t& operator= (channel_endpoint_t const&)=delete;

  // create a duplicate of the current endpoint refering
  // to the same channel. Returns a chan_epref_t; a shared
  // pointer to the new endpoint. Increments the counter
  // of endpoints in the channel.
  // note, C++ must construct a single object containing
  // both the reference count and the channel endpoint.

  ::std::shared_ptr<channel_endpoint_t> dup() const { 
    return ::std::make_shared<channel_endpoint_t>(channel);
  }

  ~channel_endpoint_t () {
// ::std::cout << "Channel endpoint " << this << " destructor" << ::std::endl; 
    switch (channel->refcnt.load()) {
      case 0: break;
      case 1: delete_channel(); break;
      default: --channel->refcnt; break;
    }
  }

  void delete_channel() {
// ::std::cout << "Delete channel " << this << ::std::endl;
    fibre_t *top = channel->top;
    channel->top = nullptr;
    channel->refcnt = 0;
    while (top) {
      fibre_t *f = (fibre_t*)clear_lowbit(top);
      fibre_t *tmp = f->next;
      delete f;
      top = tmp;
    }
  }
  
};

// channel endpoint reference type
// note, the refcnt is not atomic.
// this is fine, because endpoints belong exclusively
// to a single fibre, which cannot be executed by more
// the one thread at once.

using chan_epref_t = ::std::shared_ptr<channel_endpoint_t>;

