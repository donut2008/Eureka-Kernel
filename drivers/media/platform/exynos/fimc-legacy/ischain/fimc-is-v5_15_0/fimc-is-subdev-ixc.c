--

Return a tuple containing values unpacked according to the format string.

The buffer's size, minus offset, must be at least calcsize(format).

See help(struct) for more on format strings.              Struct(fmt) --> compiled struct object

        iter_unpack($self, buffer, /)
--

Return an iterator yielding tuples.

Tuples are unpacked from the given bytes source, like a repeated
invocation of unpack_from().

Requires that the bytes length be a multiple of the struct size.          S.pack(v1, v2, ...) -> bytes

Return a bytes object containing values v1, v2, ... packed according
to the format string S.format.  See help(struct) for more on format
strings. S.pack_into(buffer, offset, v1, v2, ...)

Pack the values v1, v2, ... according to the format string S.format
and write the packed bytes into the writable buffer buf starting at
offset.  Note that the offset is a required argument.  See
help(struct) for more on format strings.           unpack($self, buffer, /)
--

Return a tuple containing unpacked values.

Unpack according to the format string Struct.format. The buffer's size
in bytes must be Struct.size.

See help(struct) for more on format strings.     unpack_from($self, /, buffer, offset=0)
--

Return a tuple containing unpacked values.

Values are unpacked according to the format string Struct.format.

The buffer's size in bytes, starting at position offset, must be
at least Struct.size.

See help(struct) for more on format strings. S.__sizeof__() -> size of S in memory, in bytes symtable($module, source, filename, startstr, /)
--

Return symbol and scope dictionaries used internally by compiler.          A lock object is a synchronization primitive.  To create a lock,
call threading.Lock().  Methods are:

acquire() -- lock the lock, possibly blocking until it can be obtained
release() -- unlock of the lock
locked() -- test whether the lock is currently locked

A lock is not owned by the thread that locked it; another thread may
unlock it.  A thread attempting to lock a lock that it has already locked
will block until another thread unlocks it.  Deadlocks may ensue.           acquire(blocking=True, timeout=-1) -> bool
(acquire_lock() is an obsolete synonym)

Lock the lock.  Without argument, this blocks if the lock is already
locked (even by the same thread), waiting for another thread to release
the lock, and return True once the lock is acquired.
With an argument, this will only block if the argument is true,
and the return value reflects whether the lock is acquired.
The blocking operation is interruptible.      release()
(release_lock() is an obsolete synonym)

Release the lock, allowing another thread that is blocked waiting for
the lock to acquire the lock.  The lock must be in the locked state,
but it needn't be locked by the same thread that unlocks it.      locked() -> bool
(locked_lock() is an obsolete synonym)

Return whether the lock is in the locked state.        acquire(blocking=True) -> bool

Lock the lock.  `blocking` indicates whether we should wait
for the lock to be available or not.  If `blocking` is False
and another thread holds the lock, the method will return False
immediately.  If `blocking` is True and another thread holds
the lock, the method will wait for the lock to be released,
take it and then return True.
(note: the blocking operation is interruptible.)

In all other cases, the method will return True immediately.
Precisely, if the current thread already holds the lock, its
internal counter is simply incremented. If nobody holds the lock,
the lock is taken and its internal counter initialized to 1.      release()

Release the lock, allowing another thread that is blocked waiting for
the lock to acquire the lock.  The lock must be in the locked state,
and must be locked by the same thread that unlocks it; otherwise a
`RuntimeError` is raised.

Do note that if the lock was acquire()d several times in a row by the
current thread, release() needs to be called as many times for the lock
to be available for other threads.            _is_owned() -> bool

For internal use by `threading.Condition`. _acquire_restore(state) -> None

For internal use by `threading.Condition`.     _release_save() -> tuple

For internal use by `threading.Condition`.            ExceptHookArgs

Type used to pass arguments to threading.excepthook.            This module provides primitive operations to write multi-threaded programs.
The 'threading' module provides a more convenient interface.        start_new_thread(function, args[, kwargs])
(start_new() is an obsolete synonym)

Start a new thread and return its identifier.  The thread will call the
function with positional arguments from the tuple args and keyword arguments
taken from the optional dictionary kwargs.  The thread exits when the
function returns; the return value is ignored.  The thread will also exit
when the function raises an unhandled exception; a stack trace will be
printed unless the exception is SystemExit.
       allocate_lock() -> lock object
(allocate() is an obsolete synonym)

Create a new lock object. See help(type(threading.Lock())) for
information about locks.     exit()
(exit_thread() is an obsolete synonym)

This is synonymous to ``raise SystemExit''.  It will cause the current
thread to exit silently unless the exception is caught.   interrupt_main()

Raise a KeyboardInterrupt in the main thread.
A subt