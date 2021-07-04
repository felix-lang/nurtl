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

  // required for the polymorphic deleter
  virtual size_t size() const = 0;

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
  virtual void read(void **target, fibre_t **pcurrent)=0;

  // channel write operation
  virtual void write(void **source, fibre_t **pcurrent)=0;

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

struct chan_epref_t {
  channel_endpoint_t *endpoint;
  chan_epref_t(channel_endpoint_t *p) : endpoint(p) {} // endpoint ctor sets refcnt to 1 initially

  channel_endpoint_t *operator->()const { return endpoint; }
  channel_endpoint_t *get() const { return endpoint; }

  chan_epref_t() : endpoint(nullptr) {}

  // rvalue move
  chan_epref_t(chan_epref_t &&p) {
    if(p.endpoint) { 
      endpoint = p.endpoint; 
      p.endpoint = nullptr; 
    }
    else endpoint = nullptr;
  } 

  // lvalue copy
  chan_epref_t(chan_epref_t &p);

  // rvalue assign
  void operator= (chan_epref_t &&p) {
    if (&p!=this) { // ignore self assign
      this->~chan_epref_t(); // destroy target
      new(this) chan_epref_t(::std::move(p)); // move source to target
    }
  } // rass

  // lvalue assign
  void operator= (chan_epref_t &p) { 
    if(&p!=this) { // ignore self assign
      this->~chan_epref_t(); // destroy target
      new(this) chan_epref_t(p); // copy source to target
    }
  } // lass
 
 
  // destructor
  // uses allocator passed to endpoint on construction to delete it
  ~chan_epref_t();
};


// the allocator used to construct the endpoint
// must be passed to it so it can be used to delete it
// it MUST be the same allocator used to construct the channel
// try to FIXME so it is .. this requires storing the allocator in
// the channel OR ensuring all channels and endpoints are constructed
// using the system allocator
struct channel_endpoint_t {
  size_t refcnt;
  channel_t *channel;
  allocator_t *allocator;
  channel_endpoint_t(channel_t *p, allocator_t *a) : allocator(a), channel(p), refcnt(1) { ++channel->refcnt; }

  // immobile object
  channel_endpoint_t(channel_endpoint_t const&)=delete;
  channel_endpoint_t& operator= (channel_endpoint_t const&)=delete;

  // create a duplicate of the current endpoint refering
  // to the same channel. Returns a chan_epref_t; a shared
  // pointer to the new endpoint. Incredrummoyne@corleonemarinas.comments the counter
  // of endpoints in the channel.
  // note, C++ must construct a single object containing
  // both the reference count and the channel endpoint.

  chan_epref_t dup() const { 
    return chan_epref_t(new(*allocator) channel_endpoint_t(channel, allocator));
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
      delete_concrete_object(f, allocator);
      top = tmp;
    }
  }
  
};


// lvalue copy
chan_epref_t::chan_epref_t(chan_epref_t &p) { 
  if(p.endpoint) { 
    endpoint = p.endpoint; 
    p.endpoint->refcnt++; 
  }
  else endpoint = nullptr; 
} 

// destructor
// uses allocator passed to endpoint on construction to delete it
chan_epref_t::~chan_epref_t() { 
  if (endpoint) {
    if(endpoint->refcnt == 1) 
      delete_concrete_object(endpoint, endpoint->allocator); 
    else 
      --endpoint->refcnt; 
  }
} // dtor

chan_epref_t acquire_channel(alloc_ref_t a, channel_t *p) {
  return chan_epref_t(new(a) channel_endpoint_t(p,a.get()));
}

