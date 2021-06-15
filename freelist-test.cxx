// clang++ -std=c++17 -I./src/lib -O2 freelist-test.cxx
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include "freelist.hpp"

#define NUM_THREADS 16
#define NUM_ENTRIES 16
#define NUM_ITERS 0x100000

freelist_t *freelist = NULL;

void bar(int thread_no)
{
  for(long i = 0; i < NUM_ITERS; i++) {
    freelist->push(freelist->pop());
  }
}

int main() 
{
  std::thread *threads[NUM_THREADS];

  void *freelist_entries[NUM_ENTRIES];

  for (int i = 0; i < NUM_ENTRIES; i++) {
    freelist_entries[i] = reinterpret_cast<void*>(i+1);
    std::cout << "before[" << i << "] = " << freelist_entries[i] << "\n";
  }

  freelist = new freelist_t(NUM_THREADS, freelist_entries);
  freelist->top = freelist_entries; // force stack to be considered full at start

  for (int i = 0; i < NUM_THREADS; i++) {
    threads[i] = new std::thread(bar, i);
  }


  // synchronize threads:
  for (int i = 0; i < NUM_THREADS; i++) {
    threads[i]->join();
    delete threads[i];
  }

  for(int i = 0; i < NUM_ENTRIES; i++) {
    std::cout << "after[" << i << "] = " << freelist_entries[i] << std::endl;
  }

  return 0;
}
