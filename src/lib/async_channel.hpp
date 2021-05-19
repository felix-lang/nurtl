
// ASYNCHRONOUS CHANNEL
//
// This is like a concurrent channel, except 
// (a) it actively notifies possibly sleeping subscribers 
// to the condition variable // that the channel has changed state.
// (b) It increments the async count of an active set when a fibre
// of that set is pushed onto the channel
// (c) decrements the async count when a fibre previously on the channel
// is made currect or put onto an active set
//
// NOTE: when a fibre tries to do I/O on a channel and is the
// only holder of an endpoint, the reference count will be 1.
// In this case, it must be deleted because the I/O request can never be
// satisfied. In turn this would decrement the reference count to 0,
// so the channel, and all fibres on it, also need to be deleted.
// Fibres on the channel may hold endpoints to the channel,
// so if the reference count goes to zero no action is taken,
// the channel is going to be deleted anyhow.
//
// There is no point signaling subscribers to the condition variable,
// because the purpose of that is to wake up readers and
// writers that the channel state has changed, in particular, an
// unsatisfied I/O request may have been performed, causing a fibre
// on the channel to now go onto an active set and be available for
// resumption.
//
// It is important to note that external devices such as a clock
// MUST prevent this by holding an endpoint to the channel.
// In particular a clock, for example, is considered active even if
// it is sleeping waiting for an alarm to go off or a request to come in.
// A clock holds a request channel endpoint, even when there are no
// clients.

struct async_channel_t : concurrent_channel_t {
  ::std::condition_variable cv;
  ::std::mutex cv_lock;

  void signal() override { cv.notify_all(); }

  async_channel_t () {}

  void read(void **target, fibre_t **pcurrent)  override {
    fibre_t *current = *pcurrent;
    ++current->owner->async_count;
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
        return; // to prevent signalling a deleted channel
      } else {
        --refcnt;
        st_push_reader(current);
        unlock();
      }
      *pcurrent = current->owner->pop(); // active list
    }
    signal();
  }

  void write(void **source, fibre_t **pcurrent) override  {
    fibre_t *current = *pcurrent;
    ++current->owner->async_count;
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
        return; // to prevent signalling a deleted channel
      } else {
        --refcnt;
        st_push_writer(current); // i/o fail: push current onto channel
        unlock();
        *pcurrent = current->owner->pop(); // reset current from active list
      }
    }
    signal();
  }


};

chan_epref_t acquire_async_channel(async_channel_t *p) {
  return ::std::make_shared<channel_endpoint_t>(p);
}
chan_epref_t make_async_channel() {
  return acquire_async_channel(new async_channel_t);
}

