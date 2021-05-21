struct global_t {
  ::std::unique_ptr<csp_clock_t> system_clock;
  global_t() : system_clock(new csp_clock_t) {}
};

