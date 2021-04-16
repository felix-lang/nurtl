// channel4.hpp

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
  ::std::atomic_flag lock;
  
  channel_t () : top (nullptr), lock(false) {}

  // immobile object
  channel_t(channel_endpoint const&)=delete;
  channel_t& operator= (channel_endpoint const&)=delete;

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

struct channel_endpoint_t {
  channel_t *channel;
  channel_endpoint_t(channel_t *p) : channel(p) { ++channel.refcnt; }

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
    return make_shared<channel_endpoint_t>(channel);
  }

  ~channel_endpoint_t () {
    switch (channel->refcnt.load()) {
      case 0: break;
      case 1: delete_channel(); break;
      default: --channel->refcnt; break;
    }
  }

  void delete_channel() {
    fibre_t *top = channel->top;
    channel->top = nullptr;
    channel.refcnt = 0;
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

chan_epref_t make_channel() {
}

