  //
  // if there are several schedulers we must keep all of them alive
  // looping through the delay if neccessary, until
  // (A) ALL schedulers have no current work
  // (B) the active set is empty
  // (C) the async count is zero
struct pqreq_t {
  double alarm_time;
  fibre_t *fibre;
};

// note, reverse order since queue put largest on top
bool operator < (pqreq_t const &a, pqreq_t const &b) { 
  return b.alarm_time < a.alarm_time; 
}

struct csp_clock_t;
void run_service(csp_clock_t *p);

struct csp_clock_t {
  chan_epref_t chanepr;

  ::std::priority_queue<pqreq_t> q;

  bool run;
  ::std::thread thread;

  double now () { 
    auto t = ::std::chrono::high_resolution_clock().now();
    auto d = t.time_since_epoch(); // duration
    return (::std::chrono::duration<double> (d)).count();
  }

  csp_clock_t () : run(false) { 
    chanepr = make_async_channel(); 
    start(); 
  }
  ~csp_clock_t() { 
    // ::std::cerr << "Clock destructor" << ::std::endl; 
    stop(); 
  }

  chan_epref_t connect() { return chanepr->dup(); }

private:
  // puts f back on its active set after timeout
  void activate_fibre(fibre_t *f) {
    //::std::cerr << "Wake fibre" << ::std::endl;
    f->owner->push(f);
    f->owner->async_complete(); // dec async_count and signal scheduler
  }

public:
  void service() {
    //::std::cerr << ::std::fixed << "Clock service started run flag = " << run << ::std::endl;

    // This is a hack, but it is perfectly safe because WE constructed
    // the channel!
    async_channel_t *chan = reinterpret_cast<async_channel_t*>(chanepr->channel);
    while(run) {
      //::std::cerr << "Clock service iteration" << ::std::endl;

      // step 2, read any requests
      
      fibre_t *w = chan->pop_writer();

      // move any alarm requests from channel to priority queue
      while(w) {
        //::std::cerr << "Got sleep request" << ::std::endl;
        double **ppalarmat = (double**)w->cc->svc_req->io_request.pdata;
        //::std::cerr << "Address of data word " << ppalarmat << ::std::endl;
        double *palarmat = *ppalarmat;
        //::std::cerr << "data word " << palarmat << ::std::endl;
        double alarmat = *palarmat; 
        //::std::cerr << "Sleep request found on channel, alarm at " << alarmat << "!" << ::std::endl;
        //::std::cerr << "Delay = " << alarmat - now() << " seconds" << ::std::endl;
        q.push(pqreq_t{alarmat,w});

        w = chan->pop_writer();
      }

      // activate any fibres that have reached alarm time
      double t = now();
      //::std::cerr << "Time now is " << t << ::std::endl;
      double sleep_until = t + 10.0; // ten second poll
      while(!q.empty()) {
        //::std::cerr << "Examining queue" << ::std::endl;
        pqreq_t top = q.top();
        if(top.alarm_time > t) { 
           sleep_until = top.alarm_time; 
           //::std::cerr << "UNEXPRIRED TIMER" << ::std::endl;
           break; 
        }
        q.pop();
        //::std::cerr << "EXPRIRED TIMER for fibre " << top.fibre << ::std::endl;
        activate_fibre(top.fibre);
      }

      // sleep
      //::std::cerr << "Sleep until " << sleep_until << ", for " << sleep_until - now() << ::std::endl;
      {
        ::std::unique_lock lk(chan->cv_lock); // lock mutex
        auto t = ::std::chrono::time_point<
           ::std::chrono::high_resolution_clock,
           ::std::chrono::duration<double>> ( 
           ::std::chrono::duration<double>(sleep_until));

        chan->cv.wait_until(lk, t);
        // mutex is lock on exit from wait but then released by RAII
       }
       //::std::cerr << "Condition variable woke up" << ::std::endl;
    } // infinite loop
  } // service
   
  void start() {
    //::std::cerr << "Start clock, flag = " << run << ::std::endl;
    if(run) return; // already running
    run = true;
    //::std::cerr << "spawning thread, flag = " << run << ::std::endl;
    thread = ::std::thread(run_service, this); // start thread
    //::std::cerr << "thread spawned, flag = " << run << ::std::endl;
  }
  void stop () {
    //::std::cerr << "Stop clock flag = " << run << ::std::endl;
    if(run) {
      run = false;
      thread.join();
    }
  }
};

void run_service(csp_clock_t *p) { p->service(); }

// makes a clock and starts it running
// the clock dies when the last reference is lost
::std::shared_ptr<csp_clock_t> make_clock() { 
  return ::std::make_shared<csp_clock_t>();
}
