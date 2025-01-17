an error occurs in the child process before the exec, it is
serialized and written to the errpipe_write fd per subprocess.py.

Returns: the child process's PID.

Raises: Only on an error in the parent process.
            C implementation of the Python queue module.
This module is an implementation detail, please do not use it directly.            SimpleQueue()
--

Simple, unbounded, reentrant FIFO queue.      empty($self, /)
--

Return True if the queue is empty, False otherwise (not reliable!).         get($self, /, block=True, timeout=None)
--

Remove and return an item from the queue.

If optional args 'block' is true and 'timeout' is None (the default),
block if necessary until an item is available. If 'timeout' is
a non-negative number, it blocks at most 'timeout' seconds and raises
the Empty exception if no item was available within that time.
Otherwise ('block' is false), return an item if one is immediately
available, else raise the Empty exception ('timeout' is ignored
in that case).              get_nowait($self, /)
--

Remove and return an item from the queue without blocking.

Only get an item if one is immediately available. Otherwise
raise the Empty exception.     put($self, /, item, block=True, timeout=None)
--

Put the item on the queue.

The optional 'block' and 'timeout' argu