// active_set.hpp

// active set
struct active_set_t {
  ::std::atomic_size_t refcnt;
  fibre_t *active;
  ::std::atomic_flag lock;
  active_set_t() : refcnt(1), active(nullptr), lock(false) {}

  active_set_t *share() { ++refcnt; return this; }
  void forget() { --refcnt; if(!atomic_load(&refcnt)) delete this; }

  // push a new active fibre onto active list
  void push(fibre_t *fresh) { 
    while(lock.test_and_set(::std::memory_order_acquire)); // spin
    fresh->next = active; 
    active = fresh; 
    lock.clear(::std::memory_order_release); // release lock
  }

  void push_new(fibre_t *fresh) {
    fresh->owner = this;
    push(fresh);
  }

  // pop an active fibre off the active list
  fibre_t *pop() {
    while(lock.test_and_set(::std::memory_order_acquire)); // spin
    fibre_t *tmp = active;
    if(tmp)active = tmp->next;
    lock.clear(::std::memory_order_release); // release lock
    return tmp;
  }
};


