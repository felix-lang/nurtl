// The Garbage Collecting Allocators is a wrapper around
// the system garbage collector.
//
// Allocate method functions as expected, allocating a block.
// A collection may be triggered.
//
// The deallocate method deletes a block as usual.
// This method can be called by reference counting pointers
// safely. Note weak pointers may dangle.
//
// All managed memory is released by the destructor.
//
// There are two other methods; acquire and release.
// Acquire adopts a block into management, and release
// removes one. These are NOT SAFE.
//
// The actual garbage collector itself must delegate to
// another allocator to acquire memory. Acquire should
// only be used to acquire a block allocated by that
// allocator UNLESS it is released before it is collected.
// Otherwise a collection might call the wrong deallcation.
//
// This class should wrap the Felix allocator. This is a BIG JOB
// because of the heavy integration with the system, including
// threads, world stopping, and "legacy" Felix system tech.
//
