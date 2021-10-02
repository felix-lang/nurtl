// csp_thread_t4.hpp

// scheduler
struct csp_thread_t {
  csp_process_t *process;  // chain of fibres ready to run

  fibre_t *current; // currently running fibre, nullptr if none
  ~csp_thread_t() { process->forget(); }

  csp_thread_t(csp_process_t *a) : current(nullptr), process(a) {}

  void sync_run(con_t *);
  void do_read(io_request_t *req);
  void do_write(io_request_t *req);
  void do_spawn_fibre(spawn_fibre_request_t *req);
  void do_spawn_fibre_deferred(spawn_fibre_request_t *req);
  void do_spawn_process(spawn_process_request_t *req);
  void do_spawn_cothread(spawn_fibre_request_t *req);
};

extern void csp_run(system_t *system, alloc_ref_t process_allocator, con_t *init) {
::std::cerr << "csp_run start" << ::std::endl;
  csp_thread_t (new(system->system_allocator) csp_process_t(system, process_allocator)).sync_run(init);
::std::cerr << "csp_run over " << ::std::endl;
}

// scheduler subroutine runs until there is no work to do
void csp_thread_t::sync_run(con_t *cc) {
  current = new(process->process_allocator) fibre_t(cc, process);
  cc->fibre = current;
  ++process->running_thread_count;
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
        case spawn_process_request_code_e:  
          do_spawn_process(&(svc_req->spawn_process_request));
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
      //::std::cerr << "csp_thread: null continuation in fibre, deleting fibre " << current << ::std::endl;
      auto old_current = current;
      current = nullptr;
      delete_concrete_object(old_current,old_current->process->process_allocator);
      current = process->pop();      // get more work
      //::std::cerr << "csp_thread: new current fibre " << current << ::std::endl;
    }
  }

  // decrement running thread count
  process->running_thread_count--;

  // Async events can reload the active set, but they do NOT change current
rewait:
  // if the async count > 0 we're waiting for the async op to complete
  // if the running thread count > 0 we're waiting for other threads to stall
//  //::std::cerr << "Scheduler out of fibres: async count = " << process->async_count.load() << ::std::endl;
  if(process->async_count.load() > 0 || process->running_thread_count.load() > 0) {
    // delay
    {
////::std::cerr << "Scheduler sleeping (inf)" << ::std::endl;
      ::std::unique_lock<::std::mutex> lk(process->async_lock);
      process->async_wake.wait_for(lk,::std::chrono::milliseconds(100000));
    } // lock released now
    current = process->pop();      // get more work
    if(current) {
      process->running_thread_count++;
      goto retry;
    }
    goto rewait;
  }

//  //::std::cerr << "Scheduler out of work, returning" << ::std::endl;
}


void csp_thread_t::do_read(io_request_t *req) {
  req->chan->get()->channel->read(current->svc_req->io_request.pdata, &current);
}

void csp_thread_t::do_write(io_request_t *req) {
 req->chan->get()->channel->write(current->svc_req->io_request.pdata, &current);
}


void csp_thread_t::do_spawn_fibre(spawn_fibre_request_t *req) {
// //::std::cerr << "do spawn" << ::std::endl;
  current->svc_req=nullptr;
  process->push(current);
  con_t *cc= req->tospawn;
  current = new(process->process_allocator) fibre_t(cc, process);
  cc->fibre = current;
// //::std::cerr << "spawned " << current << ::std::endl;
}

void csp_thread_t::do_spawn_fibre_deferred(spawn_fibre_request_t *req) {
// //::std::cerr << "do spawn deferred" << ::std::endl;
  current->svc_req=nullptr;
  con_t *init = req->tospawn;
  fibre_t *d = new(process->process_allocator) fibre_t(init, process);
  init->fibre = d;
  process->push(d);
// //::std::cerr << "spawn deferred " << d << ::std::endl;
}

static void spawn(csp_process_t *pa, con_t *cc) {
  csp_thread_t(pa).sync_run(cc);
}
void csp_thread_t::do_spawn_process(spawn_process_request_t *req) {
  csp_process_t *process = new(process->system->system_allocator) csp_process_t(process->system, req->process_allocator);
  ::std::thread(spawn,process,req->tospawn).detach();
}

void csp_thread_t::do_spawn_cothread(spawn_fibre_request_t *req) {
  current->process->refcnt++;
  ::std::thread(spawn,current->process,req->tospawn).detach();
}


