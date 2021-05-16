// SINGLE THREADED CHANNEL (no locking)

// channel
struct sequential_channel_t : channel_t {
  
  sequential_channel_t () : channel_t() {}

  void push_reader(fibre_t *r) final { st_push_reader(r); } 
  void push_writer(fibre_t *w) final { st_push_writer(w); }
  fibre_t *pop_reader() final { return st_pop_reader(); }
  fibre_t *pop_writer() final { return st_pop_writer(); }

  void signal() final {} 

  void read(void **target, fibre_t **pcurrent) final {
    fibre_t *current = *pcurrent;
    fibre_t *w = st_pop_writer();
    if(w) {
      ++refcnt;
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
      }
      *pcurrent = current->owner->pop(); // active list
    }
  }

  void write(void **source, fibre_t **pcurrent) final {
    fibre_t *current = *pcurrent;
    fibre_t *r = st_pop_reader();
    if(r) {
      ++refcnt;
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
        *pcurrent = current->owner->pop(); // reset current from active list
      }
    }
  }




};

chan_epref_t make_sequential_channel() {
  return ::std::make_shared<channel_endpoint_t>(new sequential_channel_t);
}

