// sync_sched4.hpp

// scheduler
struct sync_sched {
  fibre_t *current; // currently running fibre, nullptr if none
  active_set_t *active_set;  // chain of fibres ready to run
  ~sync_sched() { active_set->forget(); }

  sync_sched(active_set_t *a) : current(nullptr), active_set(a) {}

  void sync_run(con_t *);
  void do_read(io_request_t *req);
  void do_write(io_request_t *req);
  void do_spawn_fibre(spawn_fibre_request_t *req);
  void do_spawn_fibre_deferred(spawn_fibre_request_t *req);
  void do_spawn_pthread(spawn_fibre_request_t *req);
  void do_spawn_cothread(spawn_fibre_request_t *req);
};

extern void csp_run(con_t *init) {
  sync_sched (new active_set_t).sync_run(init);
}

// scheduler subroutine runs until there is no work to do
void sync_sched::sync_run(con_t *cc) {
  current = new fibre_t(cc, active_set);
  cc->fibre = current;
  ++active_set->running_thread_count;
retry:
  while(current) // while there's work to do 
  {
    current->svc_req = nullptr; // null out service request
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
        case spawn_fibre_deferred_request_code_e:  
          do_spawn_fibre_deferred(&(svc_req->spawn_fibre_request));
          break;
        case spawn_pthread_request_code_e:  
          do_spawn_pthread(&(svc_req->spawn_fibre_request));
          break;
        case spawn_cothread_request_code_e:  
          do_spawn_cothread(&(svc_req->spawn_fibre_request));
          break;
        default:
          assert(false);
      }
    else // the fibre returned without issuing a request so should be dead
    {
      assert(!current->cc); // check it's adead fibre
      delete current;       // delete dead fibre
      current = active_set->pop();      // get more work
    }
  }

  // decrement running thread count
  active_set->running_thread_count--;

  // Async events can reload the active set, but they do NOT change current
rewait:
  // if the async count > 0 we're waiting for the async op to complete
  // if the running thread count > 0 we're waiting for other threads to stall
  ::std::cerr << "Scheduler out of fibres: async count = " << active_set->async_count.load() << ::std::endl;
  if(active_set->async_count.load() > 0 || active_set->running_thread_count.load() > 0) {
    // delay
    {
::std::cerr << "Scheduler sleeping (inf)" << ::std::endl;
      ::std::unique_lock<::std::mutex> lk(active_set->async_lock);
      active_set->async_wake.wait_for(lk,::std::chrono::milliseconds(100000));
    } // lock released now
    current = active_set->pop();      // get more work
    if(current) {
      active_set->running_thread_count++;
      goto retry;
    }
    goto rewait;
  }

  ::std::cerr << "Scheduler out of work, returning" << ::std::endl;
}


void sync_sched::do_read(io_request_t *req) {
  req->chan->get()->channel->read(current->svc_req->io_request.pdata, &current);
}

void sync_sched::do_write(io_request_t *req) {
 req->chan->get()->channel->write(current->svc_req->io_request.pdata, &current);
}


void sync_sched::do_spawn_fibre(spawn_fibre_request_t *req) {
// ::std::cout << "do spawn" << ::std::endl;
  current->svc_req=nullptr;
  active_set->push(current);
  con_t *cc= req->tospawn;
  current = new fibre_t(cc, active_set);
  cc->fibre = current;
// ::std::cout << "spawned " << current << ::std::endl;
}

void sync_sched::do_spawn_fibre_deferred(spawn_fibre_request_t *req) {
// ::std::cout << "do spawn deferred" << ::std::endl;
  current->svc_req=nullptr;
  con_t *init = req->tospawn;
  fibre_t *d = new fibre_t(init, active_set);
  init->fibre = d;
  active_set->push(d);
// ::std::cout << "spawn deferred " << d << ::std::endl;
}

static void spawn(active_set_t *pa, con_t *cc) {
  sync_sched(pa).sync_run(cc);
}
void sync_sched::do_spawn_pthread(spawn_fibre_request_t *req) {
  ::std::thread(spawn,new active_set_t,req->tospawn).detach();
}

void sync_sched::do_spawn_cothread(spawn_fibre_request_t *req) {
  current->owner->refcnt++;
  ::std::thread(spawn,current->owner,req->tospawn).detach();
}


