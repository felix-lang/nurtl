
// svc4.hpp
// service requests
enum svc_code_t {
  read_request_code_e,
  write_request_code_e,
  spawn_fibre_request_code_e
};
struct io_request_t {
  svc_code_t svc_code;
  channel_t *chan;
  void **pdata;
};
struct spawn_fibre_request_t {
  svc_code_t svc_code;
  con_t *tospawn;
};
union svc_req_t {
  io_request_t io_request;
  spawn_fibre_request_t spawn_fibre_request;
  svc_code_t get_code () const { return io_request.svc_code; }
};


