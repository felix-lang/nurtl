# Communicating Sequential Processes
## Architrecture
This is the new real time kernel implementing Communicating Sequential Processes.
With this software a *system* is a collection of *processes* which communicate
using *channels*. Systems can communicate with each other and the environment
by connecting to *devices*.

The unit of modularity is the *routine*. In a single threaded system, a routine
is a *coroutine* which when spawned implements a *fibre of control*, fibres are
cooperatively multi-tasked and not pre-empted.

In a multithreaded environment, a fibres can execute *concurrently*.

The system allows the construction of many allocators with distinct
properties suited to their intended use. The *system allocator* allocates
all channels. Typical coroutines are infinite loops and are ideal for
high performance signal processing.

Channels are owned by *channel endpoints* which in turn are accessed
by smart pointers called *channel endpoint references*, 
the machinery provides two level  reference counting in a unique way which 
ensures a fibre is terminated when I/O operations cannot succeed.

Channel I/O provides a higher level mechanism to implement *continuation
passing*. A *continuation* is a routine which has been *suspened* due to one
of three operations: **reading** a channel, **writing** a channel, or **spawning**
a new fibre. Suspended continuations waiting for I/O are made active after
an I/O operation synchronises the reader and writer. When a fibre spawns
another both are active. A *scheduler* picks an active fibre to *resume*
and continues executing it where it previously left off.

A fibre termintes by **suicide**, or by reading or writing a channel
with only one endpoint (which it owns). In the latter case the fibre is
**starved** or **blocked**, respectively.


