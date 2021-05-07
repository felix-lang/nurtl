// active set
struct active_set_t {
  ::std::atomic_size_t refcnt;
  fibre_t *active;
  ::std::atomic_flag lock; // this one is a spin lock for sync ops

  // an async service which pushes a fibre onto the active
  // set also decrements the active count and must 
  // signal this condition variable to wake up all schedulers
  // so they notice the active set is now populated

  ::std::atomic_size_t async_count;
  ::std::mutex async_lock; // mutex lock for async ops
  ::std::condition_variable async_wake;

  void async_complete() { --async_count; async_wake.notify_all(); }

  ::std::atomic_size_t running_thread_count;

  active_set_t() : refcnt(1), active(nullptr), async_count(0), lock(false), running_thread_count(0) { 
    // ::std::cout << "New active set" << ::std::endl;
  }

  active_set_t *share() { ++refcnt; return this; }
  void forget() { --refcnt; if(!atomic_load(&refcnt)) delete this; }

  // push a new active fibre onto active list
  void push(fibre_t *fresh) { 
// ::std::cout << "Active set push " << fresh << ::std::endl;
    while(lock.test_and_set(::std::memory_order_acquire)); // spin
    fresh->next = active; 
    active = fresh; 
    lock.clear(::std::memory_order_release); // release lock
  }
  // pop an active fibre off the active list
  fibre_t *pop() {
// ::std::cout << "Active set pop .. " << ::std::endl;
    while(lock.test_and_set(::std::memory_order_acquire)); // spin
    fibre_t *tmp = active;
    if(tmp)active = tmp->next;
    lock.clear(::std::memory_order_release); // release lock
// ::std::cout << "Active set popped .. " << tmp << ::std::endl;
    return tmp;
  }
};

