// lock free freelist
struct node_t { 
  node_t *next; 
};

struct stack
{
  std::atomic<node_t*> head;

  void push(node_t *new_node)
  {
    // put the current value of head into new_node->next
    new_node->next = head.load(std::memory_order_relaxed);

    // now make new_node the new head, but if the head
    // is no longer what's stored in new_node->next
    // (some other thread must have inserted a node just now)
    // then put that new head into new_node->next and try again
    while(!head.compare_exchange_weak ( new_node->next, new_node);
  }
  node_t *pop() 
  {
    
  }

};
