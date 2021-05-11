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
  channel_t(channel_t const&)=delete;
  channel_t& operator= (channel_t const&)=delete;

  // push a fibre as a reader: precondition it must be a reader
  // and, if the channel is non-empty it must contain only readers
  void push_reader(fibre_t *r) { 
    // ::std::cout << "channel " << this << " push reader " << r << ::std::endl;
    r->next = top; 
    top = r; 
  }

  // push a fibre as a writer: precondition it must be a writer
  // and, if the channel is non-empty it must contain only writers
  void push_writer(fibre_t *w) { 
    // ::std::cout << "channel " << this << " push writer " << w << ::std::endl;
    w->next = top; 
    top = (fibre_t*)set_lowbit(w);
  }

  // pop a reader if there is one, otherwise nullptr
  fibre_t *pop_reader() { 
// ::std::cout << "channel::pop reader" << ::std::endl;
    fibre_t *tmp = top; 
// ::std::cout << "pop reader " << tmp << ::std::endl;
    if(!tmp || get_lowbit(tmp))return nullptr;
// ::std::cout << "found reader " << tmp << ::std::endl;
    top = top -> next;
    return tmp; // lowbit is clear, its a reader 
  }

  // pop a writer if there is one, otherwise nullptr
  fibre_t *pop_writer() { 
// ::std::cout << "channel::pop writer" << ::std::endl;
    fibre_t *tmp = top; 
    if(!tmp || !get_lowbit(tmp)) return nullptr;
    tmp = (fibre_t*)clear_lowbit(tmp); // lowbit is set for writer
// ::std::cout << "found writer " << tmp << ::std::endl;
    top = tmp -> next;
    return tmp;
  }

  fibre_t *ts_pop_reader() {
    while(lock.test_and_set(::std::memory_order_acquire)); // spin
    auto r = pop_reader();
    lock.clear(::std::memory_order_release); // release lock
    return r;
  }

  fibre_t *ts_pop_writer() {
    while(lock.test_and_set(::std::memory_order_acquire)); // spin
    auto w = pop_writer();
    lock.clear(::std::memory_order_release); // release lock
    return w;
  }


};

struct async_channel_t : channel_t {
  ::std::condition_variable cv;
  ::std::mutex cvlock;


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

chan_epref_t make_channel() {
  return ::std::make_shared<channel_endpoint_t>(new channel_t);
}

