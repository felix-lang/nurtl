// CONCURRENT CHANNEL (operations are locked, but no async)

struct concurrent_channel_t : channel_t {
  ::std::atomic_flag lk;
  void lock() { while(lk.test_and_set(::std::memory_order_acquire)); }
  void unlock() { lk.clear(::std::memory_order_release); }

  concurrent_channel_t () : channel_t(), lk(false) {}

  void push_reader(fibre_t *r) final { 
    lock();
    st_push_reader(r); 
    unlock();
  } 
  void push_writer(fibre_t *w) final { 
    lock();
    st_push_writer(w); 
    unlock();
  }
  fibre_t *pop_reader() final {
    lock();
    auto r = st_pop_reader();
    unlock(); 
    return r;
  }
  fibre_t *pop_writer() final {
    lock();
    auto w = st_pop_writer();
    unlock();
    return w;
  }

  void signal() final {} 


  void read(void **target, fibre_t **pcurrent) final {
    fibre_t *current = *pcurrent;
    lock();
    fibre_t *w = st_pop_writer();
    if(w) {
      ++refcnt;
      unlock();
      *target =
        *w->cc->svc_req->io_request.pdata; // transfer data
      w->owner->push(w); // onto active list
    }
    else {
      if(refcnt == 1) {
        delete current;
      } else {
        --refcnt;
        st_push_reader(current);
        unlock();
      }
      *pcurrent = current->owner->pop(); // active list
    }
  }

  void write(void **source, fibre_t **pcurrent) final {
    fibre_t *current = *pcurrent;
    lock();
    fibre_t *r = st_pop_reader();
    if(r) {
      ++refcnt;
      unlock();
      *r->cc->svc_req->io_request.pdata = *source;

      if(r->owner == current->owner) {
        current->owner->push(current); // current is writer, pushed onto active list
        *pcurrent = r; // make reader current
      }
      else {
        r->owner->push(r);
      }
    }
    else {
      if(refcnt == 1) {
        delete current;
      } else {
        --refcnt;
  // ::std::cout<< "do_write: fibre " << current << ", set channel "<< chan <<" recnt to " << chan->refcnt << ::std::endl;
        st_push_writer(current); // i/o fail: push current onto channel
        unlock();
        *pcurrent = current->owner->pop(); // reset current from active list
      }
    }
  }


};

chan_epref_t make_concurrent_channel() {
  return ::std::make_shared<channel_endpoint_t>(new concurrent_channel_t);
}

