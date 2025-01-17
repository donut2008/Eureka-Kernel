 starting at
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
A subthread can use this function to interrupt the main thread. get_ident() -> integer

Return a non-zero integer that uniquely identifies the current thread
amongst other threads that exist simultaneously.
This may be used to identify per-thread resources.
Even though on some platforms threads identities may appear to be
allocated consecutive numbers starting at 1, this behavior should not
be relied upon, and the number should be seen purely as a magic cookie.
A thread's identity may be reused for another thread after it exits.          get_native_id() -> integer

Return a non-negative integer identifying the thread as reported
by the OS (kernel). This may be used to uniquely identify a
particular thread within a system.     _count() -> integer

Return the number of currently running Python threads, excluding
the main thread. The returned number comprises all threads created
through `start_new_thread()` as well as `threading.Thread`, and not
yet finished.

This function is meant for internal and specialized purposes only.
In most applications `threading.enumerate()` should be used instead.             stack_size([size]) -> size

Return the thread stack size used when creating new threads.  The
optional size argument specifies the stack size (in bytes) to be used
for subsequently created threads, and must be 0 (use platform or
configured default) or a positive integer value of at least 32,768 (32k).
If changing the thread stack size is unsupported, a ThreadError
exception is raised.  If the specified size is invalid, a ValueError
exception is raised, and the stack size is unmodified.  32k bytes
 currently the minimum supported stack size value to guarantee
sufficient stack space for the interpreter itself.

Note that some platforms may have particular restrictions on values for
the stack size, such as requiring a minimum stack size larger than 32 KiB or
requiring allocation in multiples of the system memory page size
- platform documentation should be referred to for more information
(4 KiB pages are common; using multiples of 4096 for the stack size is
the suggested approach in the absence of more specific information).  _set_sentinel() -> lock

Set a sentinel lock that will be released when the current thread
state is finalized (after it is untied from the interpreter).

This is a private API for the threading module.       excepthook(exc_type, exc_value, exc_traceback, thread)

Handle uncaught Thread.run() exception. getweakrefcount($module, object, /)
--

Return the number of weak references to 'object'.       _remove_dead_weakref($module, dct, key, /)
--

Atomically remove key from dict if it points to a dead weakref.  getweakrefs(object) -- return a list of all weak reference objects
that point to 'object'.      proxy(object[, callback]) -- create a proxy object that weakly
references 'object'.  'callback', if given, is called with a
reference to the proxy when 'object' is about to be finalized.      A channel ID identifies a channel and may be used as an int.    This module provides primitive operations to manage Python interpreters.
The 'interpreters' module provides a more convenient interface.        create() -> ID

Create a new interpreter and return a unique generated ID.      destroy(id)

Destroy the identified interpreter.

Attempting to destroy the current interpreter results in a RuntimeError.
So does an unrecognized ID.          list_all() -> [ID]

Return a list containing the ID of every existing interpreter.              get_current() -> ID

Return the ID of current interpreter.      get_main() -> ID

Return the ID of main interpreter.            is_running(id) -> bool

Return whether or not the identified interpreter is running.            run_string(id, script, shared)

Execute the provided string in the identified interpreter.

See PyRun_SimpleStrings.            is_shareable(obj) -> bool

Return True if the object's data may be shared between interpreters and
False otherwise.             channel_create() -> cid

Create a new cross-interpreter channel and return a unique generated ID.               channel_destroy(cid)

Close and finalize the channel.  Afterward attempts to use the channel
will behave as though it never existed.            channel_list_all() -> [cid]

Return the list of all IDs for active channels.    channel_list_interpreters(cid, *, send) -> [id]

Return the list of all interpreter IDs associated with an end of the channel.

The 'send' argument should be a boolean indicating whether to use the send or
receive end.      channel_send(cid, obj)

Add the object's data to the channel's queue.           channel_recv(cid, [default]) -> obj

Return a new object from the data at the front of the channel's queue.

If there is nothing to receive then raise ChannelEmptyError, unless
a default value is provided.  In that case return it.          channel_close(cid, *, send=None, recv=None, force=False)

Close the channel for all interpreters.

If the channel is empty then the keyword args are ignored and both
ends are immediately closed.  Otherwise, if 'force' is True then
all queued items are released and both ends are immediately
closed.

If the channel is not empty *and* 'force' is False then following
happens:

 * recv is True (regardless of send):
   - raise ChannelNotEmptyError
 * recv is None and send is None:
   - raise ChannelNotEmptyError
 * send is True and recv is not True:
   - fully close the 'send' end
   - close the 'recv' end to interpreters not already receiving
   - fully close it once empty

Closing an already closed channel results in a ChannelClosedError.

Once the channel's ID has no more ref counts in any interpreter
the channel will be destroyed.        channel_release(cid, *, send=None, recv=None, force=True)

Close the channel for the current interpreter.  'send' and 'recv'
(bool) may be used to indicate the ends to close.  By default both
ends are closed.  Closing an already closed end is a noop.      ����                                                ����       ;   Z   x   �   �   �   �     0  N              �YG �YG �YG �YG �YG �YG �YG �YG �YG �YG �YG �YG �YG �YG �ZG �ZG z[G z[G \G \G M\G M\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G �\G  `G W_G &^G �_G `G `G u_G �_G �_G ^G �_G �_G �bG �eG �eG �eG �eG �eG �bG �bG �eG �eG �bG �eG �eG �eG �eG �bG �eG �eG �eG �eG �eG �eG �eG �eG �eG �eG �eG �eG �eG �eG �eG �eG cG �eG �bG �eG �bG �eG �bG �bG �eG �eG �bG �eG �eG �eG �eG �bG �eG �eG �eG cG y|G �zG .{G H{G .{G .{G .{G i{G         This module defines an object type which can efficiently represent
an array of basic values: characters, integers, floating point
numbers.  Arrays are sequence types and behave very much like lists,
except that the type of objects stored in them is constrained.
          _array_reconstructor($module, arraytype, typecode, mformat_code, items,
                     /)
--

Internal. Used for pickling support.                                                                                                                                                                                                                                                                                                                                 array(typecode [, initializer]) -> array

Return a new array whose items are restricted by typecode, and
initialized from the optional initializer value, which must be a list,
string or iterable over elements of the appropriate type.

Arrays represent basic values and behave very much like lists, except
the type of objects stored in them is constrained. The type is specified
at object creation time by using a type code, which is a single character.
The following type codes are defined:

    Type code   C Type             Minimum size in bytes
    'b'         signed integer     1
    'B'         unsigned integer   1
    'u'         Unicode character  2 (see note)
    'h'         signed integer     2
    'H'         unsigned integer   2
    'i'         signed integer     2
    'I'         unsigned integer   2
    'l'         signed integer     4
    'L'         unsigned integer   4
    'q'         signed integer     8 (see note)
    'Q'         unsigned integer   8 (see note)
    'f'         floating point     4
    'd'         floating point     8

NOTE: The 'u' typecode corresponds to Python's unicode character. On
narrow builds this is 2-bytes on wide builds this is 4-bytes.

NOTE: The 'q' and 'Q' type codes are only available if the platform
C compiler used to build Python supports 'long long', or, on Windows,
'__int64'.

Methods:

append() -- append a new item to the end of the array
buffer_info() -- return information giving the current memory info
byteswap() -- byteswap all the items of the array
count() -- return number of occurrences of an object
extend() -- extend array by appending multiple elements from an iterable
fromfile() -- read items from a file object
fromlist() -- append items from the list
frombytes() -- append items from the string
index() -- return index of first occurrence of an object
insert() -- insert a new item into the array at a provided position
pop() -- remove and return item (default last)
remove() -- remove first occurrence of an object
reverse() -- reverse the order of the items in the array
tofile() -- write all items to a file object
tolist() -- return the array converted to an ordinary list
tobytes() -- return the array converted to a string

Attributes:

typecode -- the typecode character used to create the array
itemsize -- the length in bytes of one array item
            __reduce__($self, /)
--

Return state information for pickling. __setstate__($self, state, /)
--

Set state information for unpickling.         append($self, v, /)
--

Append new value v to the end of the array.             buffer_info($self, /)
--

Return a tuple (address, length) giving the current memory address and the length in items of the buffer used to hold array's contents.

The length should be multiplied by the itemsize attribute to calculate
the buffer length in bytes.           byteswap($self, /)
--

Byteswap all items of the array.

If the items in the array are not 1, 2, 4, or 8 bytes in size, RuntimeError is
raised. __copy__($self, /)
--

Return a copy of the array.              count($self, v, /)
--

Return number of occurrences of v in the array.          __deepcopy__($self, unused, /)
--

Return a copy of the array.  extend($self, bb, /)
--

Append items to the end of the array.  fromfile($self, f, n, /)
--

Read n objects from the file objec