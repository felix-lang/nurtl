// CONCURRENT CHANNEL (operations are locked, but no async)

struct concurrent_channel_t : sequential_channel_t {
  ::std::atomic_flag lk;
  void lock() { while(lk.test_and_set(::std::memory_order_acquire)); }
  void unlock() { lk.clear(::std::memory_order_release); }

  concurrent_channel_t () : lk(false) {}

  size_t size() const override { return sizeof(concurrent_channel_t); }

  void push_reader(fibre_t *r) override { 
    lock();
    st_push_reader(r); 
    unlock();
  } 
  void push_writer(fibre_t *w) override { 
    lock();
    st_push_writer(w); 
    unlock();
  }
  fibre_t *pop_reader() override {
    lock();
    auto r = st_pop_reader();
    unlock(); 
    return r;
  }
  fibre_t *pop_writer() override {
    lock();
    auto w = st_pop_writer();
    unlock();
    return w;
  }

  void read(void **target, fibre_t **pcurrent) override {
    fibre_t *current = *pcurrent;
    lock();
    fibre_t *w = st_pop_writer();
    if(w) {
      ++refcnt;
      unlock();
      *target =
        *w->svc_req->io_request.pdata; // transfer data
      w->process->push(w); // onto active list
    }
    else {
      if(refcnt == 1) {
        //::std::cerr << "Concurrent channel read deletes requesting fibre " << current << :: std::endl;
        *pcurrent = current->process->pop(); // active list
        delete_concrete_object(current,current->process->process_allocator);
      } else {
        --refcnt;
        st_push_reader(current);
        unlock();
        *pcurrent = current->process->pop(); // active list
      }
    }
  }

  void write(void **source, fibre_t **pcurrent) override {
    fibre_t *current = *pcurrent;
    lock();
    fibre_t *r = st_pop_reader();
    if(r) {
      ++refcnt;
      unlock();
      *r->svc_req->io_request.pdata = *source;

      if(r->process == current->process) {
        current->process->push(current); // current is writer, pushed onto active list
        *pcurrent = r; // make reader current
      }
      else {
        r->process->push(r);
      }
    }
    else {
      if(refcnt == 1) {
        //::std::cerr << "Concurrent channel write deletes requesting fibre " << current << :: std::endl;
        *pcurrent = current->process->pop(); // reset current from active list
        delete_concrete_object(current,current->process->process_allocator);
      } else {
        --refcnt;
  // ::std::cout<< "do_write: fibre " << current << ", set channel "<< chan <<" recnt to " << chan->refcnt << ::std::endl;
        st_push_writer(current); // i/o fail: push current onto channel
        unlock();
        *pcurrent = current->process->pop(); // reset current from active list
      }
    }
  }


};

chan_epref_t make_concurrent_channel(alloc_ref_t a) {
  return acquire_channel(a, new(a) concurrent_channel_t);
}

void system_t::connect_concurrent (chan_epref_t *left, chan_epref_t *right) {
  auto chleft= make_concurrent_channel(system_allocator);
  auto chright= chleft->dup(); 
  *left = chleft;
  *right= chright;
}


