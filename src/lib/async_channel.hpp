
// NOTE: channel is a subtype of async channel:
// implementation inheritance only.
struct async_channel_t : channel_t {
  ::std::condition_variable cv;
  ::std::mutex cvlock;
  void signal() { cv.notify_all(); }
};


struct async_channel_endpoint_t {
  async_channel_t *async_channel;
  async_channel_endpoint_t(async_channel_t *p) : async_channel(p) { ++async_channel->refcnt; }

  // immobile object
  async_channel_endpoint_t(async_channel_endpoint_t const&)=delete;
  async_channel_endpoint_t& operator= (async_channel_endpoint_t const&)=delete;

  ::std::shared_ptr<async_channel_endpoint_t> dup() const { 
    return ::std::make_shared<async_channel_endpoint_t>(async_channel);
  }

  ~async_channel_endpoint_t () {
    switch (async_channel->refcnt.load()) {
      case 0: break;
      case 1: delete_async_channel(); break;
      default: --async_channel->refcnt; break;
    }
  }

  void delete_async_channel() {
    fibre_t *top = async_channel->top;
    async_channel->top = nullptr;
    async_channel->refcnt = 0;
    while (top) {
      fibre_t *f = (fibre_t*)clear_lowbit(top);
      fibre_t *tmp = f->next;
      delete f;
      top = tmp;
    }
  }
  
};

using chan_epref_t = ::std::shared_ptr<async_channel_endpoint_t>;

chan_async_epref_t make_async_channel() {
  return ::std::make_shared<async_channel_endpoint_t>(new async_channel_t);
}

