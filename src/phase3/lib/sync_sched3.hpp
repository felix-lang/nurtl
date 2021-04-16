// sync_sched3.hpp

// scheduler
struct sync_sched {
  fibre_t *current; // currently running fibre, nullptr if none
  active_set_t *active_set;  // chain of fibres ready to run
  ~sync_sched() { active_set->forget(); }

  sync_sched() : current(nullptr), active_set(new active_set_t) {}
  sync_sched(active_set_t *a) : current(nullptr), active_set(a) {}

  void sync_run();
  void do_read(io_request_t *req);
  void do_write(io_request_t *req);
  void do_spawn_fibre(spawn_fibre_request_t *req);
};

// scheduler subroutine runs until there is no work to do
void sync_sched::sync_run() {
  current = active_set->pop(); // get some work
  while(current) // while there's work to do 
  {
    svc_req_t *svc_req = current->run_fibre();
    if(svc_req)  // fibre issued service request
      switch (svc_req->get_code()) 
      {
        case read_request_code_e: 
          do_read(&(svc_req->io_request));
          break;
        case write_request_code_e:  
          do_write(&(svc_req->io_request));
          break;
        case spawn_fibre_request_code_e:  
          do_spawn_fibre(&(svc_req->spawn_fibre_request));
          break;
      }
    else // the fibre returned without issuing a request so should be dead
    {
      assert(!current->cc); // check it's adead fibre
      delete current;       // delete dead fibre
      current = active_set->pop();      // get more work
    }
  }
}


void sync_sched::do_read(io_request_t *req) {
  channel_t *chan = req->chan;
  while(chan->lock.test_and_set(::std::memory_order_acquire)); // spin
  fibre_t *w = chan->pop_writer();
  if(w) {
    chan->lock.clear(::std::memory_order_release); // release lock
    *current->cc->svc_req->io_request.pdata =
      *w->cc->svc_req->io_request.pdata; // transfer data

    // null out svc requests so they're not re-issued
    w->cc->svc_req = nullptr;
    current->cc->svc_req = nullptr;

    w->owner->push(w); // onto active list
    // i/o match: reader retained as current
  }
  else {
    chan->push_reader(current);
    chan->lock.clear(::std::memory_order_release); // release lock
    current = active_set->pop(); // active list
    // i/o fail: current pushed then set to next active
  }
}

void sync_sched::do_write(io_request_t *req) {
  channel_t *chan = req->chan;
  while(chan->lock.test_and_set(::std::memory_order_acquire)); // spin
  fibre_t *r = req->chan->pop_reader();
  if(r) {
    chan->lock.clear(::std::memory_order_release); // release lock
    *r->cc->svc_req->io_request.pdata = 
      *current->cc->svc_req->io_request.pdata; // transfer data

    // null out svc requests so they're not re-issued
    r->cc->svc_req = nullptr;
    current->cc->svc_req = nullptr;

    if(r->owner == active_set) {
      active_set->push(current); // current is writer, pushed onto active list
      current = r; // make reader current
    }
    else {
      r->owner->push(r);
      // writer remains current if reader is foreign
    }
  }
  else {
    chan->push_writer(current); // i/o fail: push current onto channel
    chan->lock.clear(::std::memory_order_release); // release lock
    current = active_set->pop(); // reset current from active list
  }
}


void sync_sched::do_spawn_fibre(spawn_fibre_request_t *req) {
  req->tospawn->owner = active_set;
  current->cc->svc_req=nullptr;
  active_set->push(current);
  current = req->tospawn;
}

