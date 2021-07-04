// SINGLE THREADED CHANNEL (no locking)

// channel
struct sequential_channel_t : channel_t {
  
  sequential_channel_t () : channel_t() {}

  size_t size() const override { return sizeof(sequential_channel_t); }
  void push_reader(fibre_t *r) override { st_push_reader(r); } 
  void push_writer(fibre_t *w) override  { st_push_writer(w); }
  fibre_t *pop_reader() override  { return st_pop_reader(); }
  fibre_t *pop_writer() override  { return st_pop_writer(); }

  void signal() override  {} 

  void read(void **target, fibre_t **pcurrent) override  {
    fibre_t *current = *pcurrent;
    fibre_t *w = st_pop_writer();
    if(w) {
      ++refcnt;
      *target =
        *w->svc_req->io_request.pdata; // transfer data
      w->process->push(w); // onto active list
    }
    else {
      if(refcnt == 1) {
        delete_concrete_object(current,current->process->process_allocator);
      } else {
        --refcnt;
        st_push_reader(current);
      }
      *pcurrent = current->process->pop(); // active list
    }
  }

  void write(void **source, fibre_t **pcurrent) override  {
    fibre_t *current = *pcurrent;
    fibre_t *r = st_pop_reader();
    if(r) {
      ++refcnt;
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
        delete_concrete_object(current,current->process->process_allocator);
      } else {
        --refcnt;
  // ::std::cout<< "do_write: fibre " << current << ", set channel "<< chan <<" recnt to " << chan->refcnt << ::std::endl;
        st_push_writer(current); // i/o fail: push current onto channel
        *pcurrent = current->process->pop(); // reset current from active list
      }
    }
  }




};

chan_epref_t make_sequantial_channel(alloc_ref_t a) {
  return acquire_channel(a, new(a) sequential_channel_t);
}

