d saved group ids.         getxattr($module, /, path, attribute, *, follow_symlinks=True)
--

Return the value of extended attribute attribute on path.

path may be either a string, a path-like object, or an open file descriptor.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, getxattr will examine the symbolic link itself instead of the file
  the link points to.         setxattr($module, /, path, attribute, value, flags=0, *,
         follow_symlinks=True)
--

Set extended attribute attribute on path to value.

path may be either a string, a path-like object,  or an open file descriptor.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, setxattr will modify the symbolic link itself instead of the file
  the link points to.       removexattr($module, /, path, attribute, *, follow_symlinks=True)
--

Remove extended attribute attribute on path.

path may be either a string, a path-like object, or an open file descriptor.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, removexattr will modify the symbolic link itself instead of the file
  the link points to. listxattr($module, /, path=None, *, follow_symlinks=True)
--

Return a list of extended attributes on path.

path may be either None, a string, a path-like object, or an open file descriptor.
if path is None, listxattr will examine the current directory.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, listxattr will examine the symbolic link itself instead of the file
  the link points to.    get_terminal_size($module, fd=<unrepresentable>, /)
--

Return the size of the terminal window as (columns, lines).

The optional argument fd (default standard output) specifies
which file descriptor should be queried.

If the file descriptor is not connected to a terminal, an OSError
is thrown.

This function will only be defined if an implementation is
available for this system.

shutil.get_terminal_size is the high-level function which should
normally be used, os.get_terminal_size is the low-level implementation.       cpu_count($module, /)
--

Return the number of CPUs in the system; return None if indeterminable.

This number is not equivalent to the number of CPUs the current process can
use.  The number of usable CPUs can be obtained with
``len(os.sched_getaffinity(0))``            get_inheritable($module, fd, /)
--

Get the close-on-exe flag of the specified file descriptor. set_inheritable($module, fd, inheritable, /)
--

Set the inheritable flag of the specified file descriptor.     get_blocking($module, fd, /)
--

Get the blocking mode of the file descriptor.

Return False if the O_NONBLOCK flag is set, True if the flag is cleared.        set_blocking($module, fd, blocking, /)
--

Set the blocking mode of the specified file descriptor.

Set the O_NONBLOCK flag if blocking is False,
clear the O_NONBLOCK flag otherwise.          scandir($module, /, path=None)
--

Return an iterator of DirEntry objects for given path.

path can be specified as either str, bytes, or a path-like object.  If path
is bytes, the names of yielded DirEntry objects will also be bytes; in
all other circumstances they will be str.

If path is None, uses the path='.'.    fspath($module, /, path)
--

Return the file system path representation of the object.

If the object is str or bytes, then allow it to pass through as-is. If the
object defines __fspath__(), then return the result of that method. All other
types raise a TypeError.       waitstatus_to_exitcode($module, /, status)
--

Convert a wait status to an exit code.

On Unix:

* If WIFEXITED(status) is true, return WEXITSTATUS(status).
* If WIFSIGNALED(status) is true, return -WTERMSIG(status).
* Otherwise, raise a ValueError.

On Windows, return status shifted right by 8 bits.

On Unix, if the process is being traced or if waitpid() was called with
WUNTRACED option, the caller must first check if WIFSTOPPED(status) is true.
This function must not be called if WIFSTOPPED(status) is true.             waitid_result: Result from waitid.

This object may be accessed either as a tuple of
  (si_pid, si_uid, si_signo, si_status, si_code),
or via the attributes si_pid, si_uid, and so on.

See os.waitid for more information.    stat_result: Result from stat, fstat, or lstat.

This object may be accessed either as a tuple of
  (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)
or via the attributes st_mode, st_ino, st_dev, st_nlink, st_uid, and so on.

Posix/windows: If your platform supports st_blksize, st_blocks, st_rdev,
or st_flags, they are available as attributes only.

See os.stat for more information.   statvfs_result: Result from statvfs or fstatvfs.

This object may be accessed either as a tuple of
  (bsize, frsize, blocks, bfree, bavail, files, ffree, favail, flag, namemax),
or via the attributes f_bsize, f_frsize, f_blocks, f_bfree, and so on.

See os.statvfs for more information.  sched_param(sched_priority)
--

Currently has only one field: sched_priority

  sched_priority
    A scheduling parameter.      A tuple of (columns, lines) for holding terminal window size    is_dir($self, /, *, follow_symlinks=True)
--

Return True if the entry is a directory; cached per entry.        is_file($self, /, *, follow_symlinks=True)
--

Return True if the entry is a file; cached per entry.            is_symlink($self, /)
--

Return True if the entry is a symbolic link; cached per entry.         stat($self, /, *, follow_symlinks=True)
--

Return stat_result object for the entry; cached per entry.          inode($self, /)
--

Return inode of the entry; cached per entry.                __fspath__($self, /)
--

Returns the path for the entry.                        times_result: Result from os.times().

This object may be accessed either as a tuple of
  (user, system, children_user, children_system, elapsed),
or via the attributes user, system, children_user, children_system,
and elapsed.

See os.times for more information.         uname_result: Result from os.uname().

This object may be accessed either as a tuple of
  (sysname, nodename, release, version, machine),
or via the attributes sysname, nodename, release, version, and machine.

See os.uname for more information.           ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/                SERVERINFOV2 FOR                0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz./                http://www.w3.org/XML/1998/namespace            SERVERINFO FOR  �DX���x#U�ōr�l!�6֮I�N��#�|���������������������������?        http://www.w3.org/2000/xmlns/   0123456789ABCDEF                                invalid signal number %ld, please use valid_signals()           !"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr                This module provides mechanisms to use signal handlers in Python.

Functions:

alarm() -- cause SIGALRM after a specified time [Unix only]
setitimer() -- cause a signal (described below) after a specified
               float time and the timer may restart then [Unix only]
getitimer() -- get current value of timer [Unix only]
signal() -- set the action for a given signal
getsignal() -- get the signal action for a given signal
pause() -- wait until a signal arrives [Unix only]
default_int_handler() -- default SIGINT handler

signal constants:
SIG_DFL -- used to refer to the system default handler
SIG_IGN -- used to ignore the signal
NSIG -- number of defined signals
SIGINT, SIGTERM, etc. -- signal numbers

itimer constants:
ITIMER_REAL -- decrements in real time, and delivers SIGALRM upon
               expiration
ITIMER_VIRTUAL -- decrements only when the process is executing,
               and delivers SIGVTALRM upon expiration
ITIMER_PROF -- decrements both when the process is executing and
               when the system is executing on behalf of the process.
               Coupled with ITIMER_VIRTUAL, this timer is usually
               used to profile the time spent by the application
               in user and kernel space. SIGPROF is delivered upon
               expiration.


*** IMPORTANT NOTICE ***
A signal handler function is called with two arguments:
the first is the signal number, the second is the interrupted stack frame.           default_int_handler(...)

The default handler for SIGINT installed by Python.
It raises KeyboardInterrupt.      alarm($module, seconds, /)
--

Arrange for SIGALRM to arrive after the given number of seconds. setitimer($module, which, seconds, interval=0.0, /)
--

Sets given itimer (one of ITIMER_REAL, ITIMER_VIRTUAL or ITIMER_PROF).

The timer will fire after value seconds and after that every interval seconds.
The itimer can be cleared by setting seconds to zero.

Returns old values as a tuple: (delay, interval).         getitimer($module, which, /)
--

Returns current value of given itimer.         signal($module, signalnum, handler, /)
--

Set the action for the given signal.

The action can be SIG_DFL, SIG_IGN, or a callable Python object.
The previous action is returned.  See getsignal() for possible return values.

*** IMPORTANT NOTICE ***
A signal handler function is called with two arguments:
the first is the signal number, the second is the interrupted stack frame.    raise_signal($module, signalnum, /)
--

Send a signal to the executing process. strsignal($module, signalnum, /)
--

Return the system description of the given signal.

The return values can be such as "Interrupt", "Segmentation fault", etc.
Returns None if the signal is not recognized. getsignal($module, signalnum, /)
--

Return the current action for the given signal.

The return value can be:
  SIG_IGN -- if the signal is being ignored
  SIG_DFL -- if the default action for the signal is in effect
  None    -- if an unknown handler is in effect
  anything else -- the callable Python object used as a handler       set_wakeup_fd(fd, *, warn_on_full_buffer=True) -> fd

Sets the fd to be written to (with the signal number) when a signal
comes in.  A library can use this to wakeup select or poll.
The previous fd or -1 is returned.

The fd must be non-blocking.          siginterrupt($module, signalnum, flag, /)
--

Change system call restart behaviour.

If flag is False, system calls will be restarted when interrupted by
signal sig, else system calls will be interrupted.    pause($module, /)
--

Wait until a signal arrives.              pthread_kill($module, thread_id, signalnum, /)
--

Send a signal to a thread.   pthread_sigmask($module, how, mask, /)
--

Fetch and/or change the signal mask of the calling thread.           sigpending($module, /)
--

Examine pending signals.

Returns a set of signal numbers that are pending for delivery to
the calling thread.       sigwait($module, sigset, /)
--

Wait for a signal.

Suspend execution of the calling thread until the delivery of one of the
signals specified in the signal set sigset.  The function accepts the signal
and returns the signal number.        sigwaitinfo($module, sigset, /)
--

Wait synchronously until one of the signals in *sigset* is delivered.

Returns a struct_siginfo containing information about the signal.    sigtimedwait($module, sigset, timeout, /)
--

Like sigwaitinfo(), but with a timeout.

The timeout is specified in seconds, with floating point numbers allowed.                valid_signals($module, /)
--

Return a set of valid signal numbers on this platform.

The signal numbers returned by this function can be safely passed to
functions like `pthread_sigmask`.    struct_siginfo: Result from sigwaitinfo or sigtimedwait.

This object may be accessed either as a tuple of
(si_signo, si_code, si_errno, si_pid, si_uid, si_status, si_band),
or via the attributes si_signo, si_code, and so on.               Debug module to trace memory blocks allocated by Python.        is_tracing($module, /)
--

Return True if the tracemalloc module is tracing Python memory allocations.          clear_traces($module, /)
--

Clear traces of memory blocks allocated by Python. _get_traces($module, /)
--

Get traces of all memory blocks allocated by Python.

Return a list of (size: int, traceback: tuple) tuples.
traceback is a tuple of (filename: str, lineno: int) tuples.

Return an empty list if the tracemalloc module is disabled.              _get_object_traceback($module, obj, /)
--

Get the traceback where the Python object obj was allocated.

Return a tuple of (filename: str, lineno: int) tuples.
Return None if the tracemalloc module is disabled or did not
trace the allocation of the object.                start($module, nframe=1, /)
--

Start tracing Python memory allocations.

Also set the maximum number of frames stored in the traceback of a
trace to nframe.   stop($module, /)
--

Stop tracing Python memory allocations.

Also clear traces of memory blocks allocated by Python.           get_traceback_limit($module, /)
--

Get the maximum number of frames stored in the traceback of a trace.

By default, a trace of an allocated memory block only stores
the most recent frame: the limit is 1.   get_tracemalloc_memory($module, /)
--

Get the memory usage in bytes of the tracemalloc module.

This memory is used internally to trace memory allocations.    get_traced_memory($module, /)
--

Get the current size and peak size of memory blocks traced by tracemalloc.

Returns a tuple: (current: int, peak: int).       reset_peak($module, /)
--

Set the peak size of memory blocks traced by tracemalloc to the current size.

Do nothing if the tracemalloc module is not tracing memory allocations.   /       :                   This module provides access to the garbage collector for reference cycles.

enable() -- Enable automatic garbage collection.
disable() -- Disable automatic garbage collection.
isenabled() -- Returns true if automatic collection is enabled.
collect() -- Do a full collection right now.
get_count() -- Return the current collection counts.
get_stats() -- Return list of dictionaries containing per-generation stats.
set_debug() -- Set debugging flags.
get_debug() -- Get debugging flags.
set_threshold() -- Set the collection thresholds.
get_threshold() -- Return the current the collection thresholds.
get_objects() -- Return a list of all objects tracked by the collector.
is_tracked() -- Returns true if a given object is tracked.
is_finalized() -- Returns true if a given object has been already finalized.
get_referrers() -- Return the list of objects that refer to an object.
get_referents() -- Return the list of objects that an object refers to.
freeze() -- Freeze all tracked objects and ignore them for future collections.
unfreeze() -- Unfreeze all objects in the permanent generation.
get_freeze_count() -- Return the number of objects in the permanent generation.
         enable($module, /)
--

Enable automatic garbage collection.     disable($module, /)
--

Disable automatic garbage collection.   isenabled($module, /)
--

Returns true if automatic garbage collection is enabled.              set_debug($module, flags, /)
--

Set the garbage collection debugging flags.

  flags
    An integer that can have the following bits turned on:
      DEBUG_STATS - Print statistics during collection.
      DEBUG_COLLECTABLE - Print collectable objects found.
      DEBUG_UNCOLLECTABLE - Print unreachable but uncollectable objects
        found.
      DEBUG_SAVEALL - Save objects to gc.garbage rather than freeing them.
      DEBUG_LEAK - Debug leaking programs (everything but STATS).

Debugging information is written to sys.stderr.        get_debug($module, /)
--

Get the garbage collection debugging flags.           get_count($module, /)
--

Return a three-tuple of the current collection counts.                set_threshold(threshold0, [threshold1, threshold2]) -> None

Sets the collection thresholds.  Setting threshold0 to zero disables
collection.
  get_threshold($module, /)
--

Return the current collection thresholds.         collect($module, /, generation=2)
--

Run the garbage collector.

With no arguments, run a full collection.  The optional argument
may be an integer specifying which generation to collect.  A ValueError
is raised if the generation number is invalid.

The number of unreachable objects is returned.       get_objects($module, /, generation=None)
--

Return a list of objects tracked by the collector (excluding the list returned).

  generation
    Generation to extract the objects from.

If generation is not None, return only the objects tracked by the collector
that are in that generation.               get_stats($module, /)
--

Return a list of dictionaries containing per-generation statistics.   is_tracked($module, obj, /)
--

Returns true if the object is tracked by the garbage collector.

Simple atomic objects will return false.       is_finalized($module, obj, /)
--

Returns true if the object has been already finalized by the GC.              get_referrers(*objs) -> list
Return the list of objects that directly refer to any of objs.     get_referents(*objs) -> list
Return the list of objects that are directly referred to by objs.  freeze($module, /)
--

Freeze all current tracked objects and ignore them for future collections.

This can be used before a POSIX fork() call to make the gc copy-on-write friendly.
Note: collection before a POSIX fork() call may free pages for future allocation
which can cause copy-on-write.           unfreeze($module, /)
--

Unfreeze all objects in the permanent generation.

Put all objects in the permanent generation back into oldest generation.            get_freeze_count($module, /)
--

Return the number of objects in the permanent generation.      Module contains faster C implementation of abc.ABCMeta          get_cache_token($module, /)
--

Returns the current ABC cache token.

The token is an opaque object (supporting equality testing) identifying the
current version of the ABC cache for virtual subclasses. The token changes
with every call to register() on any ABC.          _abc_init($module, self, /)
--

Internal ABC helper for class set-up. Should be never used outside abc module.  _reset_registry($module, self, /)
--

Internal ABC helper to reset registry of a given class.

Should be only used by refleak.py                _reset_caches($module, self, /)
--

Internal ABC helper to reset both caches of a given class.

Should be only used by refleak.py               _get_dump($module, self, /)
--

Internal ABC helper for cache and registry debugging.

Return shallow copies of registry, of both caches, and
negative cache version. Don't call this function directly,
instead use ABC._dump_registry() for a nice repr.      _abc_register($module, self, subclass, /)
--

Internal ABC helper for subclasss registration. Should be never used outside abc module.          _abc_instancecheck($module, self, instance, /)
--

Internal ABC helper for instance checks. Should be never used outside abc module.            _abc_subclasscheck($module, self, subclass, /)
--

Internal ABC helper for subclasss checks. Should be never used outside abc module.           Internal state held by ABC machinery.           Accelerator module for asyncio  get_event_loop($module, /)
--

Return an asyncio event loop.

When called from a coroutine or a callback (e.g. scheduled with
call_soon or similar API), this function will always return the
running event loop.

If there is no running event loop set, the function will return
the result of `get_event_loop_policy().get_event_loop()` call.               get_running_loop($module, /)
--

Return the running event loop.  Raise a RuntimeError if there is none.

This function is thread-specific.      _get_running_loop($module, /)
--

Return the running event loop or None.

This is a low-level function intended to be used by event loops.
This function is thread-specific.    _set_running_loop($module, loop, /)
--

Set the running event loop.

This is a low-level function intended to be used by event loops.
This function is thread-specific.         _register_task($module, /, task)
--

Register a new task in asyncio as executed by loop.

Returns None.         _unregister_task($module, /, task)
--

Unregister a tas