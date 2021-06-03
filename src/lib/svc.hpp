
// service requests
//
// Request codes
enum svc_code_t {
  read_request_code_e,
  write_request_code_e,
  spawn_fibre_request_code_e,
  spawn_fibre_deferred_request_code_e,
  spawn_pthread_request_code_e,
  spawn_cothread_request_code_e
};

// synchronous I/O requests
struct io_request_t {
  svc_code_t svc_code;
  chan_epref_t *chan;
  void **pdata;
};

// fibre and pthread spawn requests
struct spawn_fibre_request_t {
  svc_code_t svc_code;
  con_t *tospawn;
};

// unified service request type (only used for casts)
union svc_req_t {
  io_request_t io_request;
  spawn_fibre_request_t spawn_fibre_request;
  svc_code_t get_code () const { return io_request.svc_code; }
};


