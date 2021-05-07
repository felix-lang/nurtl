#include "sync.hpp"
#include <cmath>

#include <iostream>

struct mic : con_t {
  chan_epref_t outchan; // smart ptr to channel endpoint
  int samples_written;
  float frequency;
  io_request_t w_req;
  float *sample;

  CSP_CALLDEF_START,
    chan_epref_t outchan_a,
    float frequency_a
  CSP_CALLDEF_MID
    outchan = outchan_a;
    frequency = frequency_a;
    samples_written = 0;
    //sample
  CSP_CALLDEF_END

  CSP_RESUME_START
    ::std::cout << "Mic input starts" << ::std::endl;
    sample = new float;
    SVC_WRITE_REQ(&w_req,&outchan,&sample);

  case 1:
      *sample = std::sin(2.0*M_PI * frequency * samples_written / 44100.0);
      SVC(&w_req);

      CSP_GOTO(1)
    CSP_RETURN

  CSP_RESUME_END
};

struct file_sink : con_t {
  FILE *file;

  io_request_t r_req;
  chan_epref_t inp;

  float *pp;

  CSP_CALLDEF_START,
    chan_epref_t inchan_a,
    FILE *file_a
  CSP_CALLDEF_MID
    inp = inchan_a;
    file = file_a;
  CSP_CALLDEF_END

  CSP_RESUME_START
    ::std::cout << "File output starts" << ::std::endl;
    SVC_READ_REQ(&r_req,&inp,&pp)
    SVC(&r_req)
  case 1:
    ::std::cout << "zzz " << *pp << ::std::endl;
    CSP_RETURN

  CSP_RESUME_END
};

struct init: con_t {
  FILE *file;

  spawn_fibre_request_t spawn_req;
  chan_epref_t ch1out;
  chan_epref_t ch1inp;
  chan_epref_t ch2out;
  chan_epref_t ch2inp;


  ~init() {}

  // store parameters in local variables
  CSP_CALLDEF_START,
    FILE *file_a
  CSP_CALLDEF_MID
    file = file_a;
  CSP_CALLDEF_END

  CSP_RESUME_START
    ch1out = make_channel();
    ch1inp = ch1out->dup(); 
    ch2out = make_channel();
    ch2inp = ch2out->dup();
 
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new mic)->call(nullptr, ch1out, 440.0f))
    SVC(&spawn_req)
 
  case 1:
    SVC_SPAWN_FIBRE_DEFERRED_REQ(&spawn_req, (new file_sink)->call(nullptr, ch1inp, file))
    SVC(&spawn_req)

  case 2:
    CSP_RETURN 


  CSP_RESUME_END
}; // init class



int main() {
  FILE *outFile = fopen("./mic-output.raw", "wb");

  csp_run((new init)-> call(nullptr, outFile));
  fclose(outFile);

} 

