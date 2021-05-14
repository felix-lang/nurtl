// SINGLE THREADED CHANNEL (no locking)

// channel
struct sequential_channel_t : channel_t {
  
  sequential_channel_t () : channel() {}

  void push_reader(fibre_t *r) final { st_push_reader(r); } 
  void push_writer(fibre_t *w) final { st_push_writer(w); }
  fibre_t *pop_reader() final { return st_pop_reader(); }
  fibre_t *pop_writer() final { return st_pop_writer(); }
  void signal() final {} 
};

chan_epref_t make_sequential_channel() {
  return ::std::make_shared<channel_endpoint_t>(new sequential_channel_t);
}

