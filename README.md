# Communicating Sequential Processes
## Architrecture
This is the new real time kernel implementing Communicating Sequential Processes.
With this software a *system* is a collection of *processes* which communicate
using *channels*. Systems can communicate with each other and the environment
by connecting to *devices*.

# Hello World

  struct hello_world : coroutine_t {
    size_t size()const { return sizeof(*this); }
    con_t *resume() {
      ::std::cout << "Hello World" << ::std::endl;
      return return_control();
    }
  };

  int main() {
    alloc_ref_t system_allocator = new malloc_free_allocator_t;
    auto process_allocator = system_allocator;
    auto hello = new(process_allocator) hello_world();
    auto system = new system_t(system_allocator);
    csp_run(system, process_allocator, hello);
    delete system;
  }


