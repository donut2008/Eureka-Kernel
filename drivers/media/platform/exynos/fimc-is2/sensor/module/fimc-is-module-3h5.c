ception by displaying it with a traceback on sys.stderr.      exit($module, status=None, /)
--

Exit the interpreter by raising SystemExit(status).

If the status is omitted or None, it defaults to zero (i.e., success).
If the status is an integer, it will be used as the system exit status.
If it is another kind of object, it will be printed and the system
exit status will be one (i.e., failure).               getdefaultencoding($module, /)
--

Return the current default encoding used by the Unicode implementation.      getdlopenflags($module, /)
--

Return the current value of the flags that are used for dlopen calls.

The flag constants are defined in the os module.          getallocatedblocks($module, /)
--

Return the number of memory blocks currently allocated.      getfilesystemencoding($module, /)
--

Return the encoding used to convert Unicode filenames to OS filenames.    getfilesystemencodeerrors($module, /)
--

Return the error mode used Unicode to OS filename conversion.         getrefcount($module, object, /)
--

Return the reference count of object.

The count returned is generally one higher than you might expect,
because it includes the (temporary) reference as an argument to
getrefcount().     getrecursionlimit($module, /)
--

Return the current value of the recursion limit.

The recursion limit is the maximum depth of the Python interpreter
stack.  This limit prevents infinite recursion from causing an overflow
of the C stack and crashing Python.              getsizeof(object [, default]) -> int

Return the size of object in bytes.       _getframe($module, depth=0, /)
--

Return a frame object from the call stack.

If optional integer depth is given, return the frame object that many
calls below the top of the stack.  If that is deeper than the call
stack, ValueError is raised.  The default for depth is zero, returning
the frame at the top of the call stack.

This function should be used for internal and specialized purposes
only.                intern($module, string, /)
--

``Intern'' the given string.

This enters the string in the (global) table of interned strings whose
purpose is to speed up dictionary lookups. Return the string itself or
the previously interned string object with the same value.           is_finalizing($module, /)
--

Return True if Python is exiting. setswitchinterval($module, interval, /)
--

Set the ideal thread switching delay inside the Python interpreter.

The actual frequency of switching threads can be lower if the
interpreter executes long sequences of uninterruptible code
(this is implementation-specific and workload-dependent).

The parameter must represent the desired switching delay in seconds
A typical value is 0.005 (5 milliseconds).            getswitchinterval($module, /)
--

Return the current thread switch interval; see sys.setswitchinterval().       setdlopenflags($module, flags, /)
--

Set the flags used by the interpreter for dlopen calls.

This is used, for example, when the interpreter loads extension
modules. Among other things, this will enable a lazy resolving of
symbols when importing a module, if called as sys.setdlopenflags(0).
To share symbols across extension modules, call as
sys.setdlopenflags(os.RTLD_GLOBAL).  Symbolic names for the flag
modules can be found in the os module (RTLD_xxx constants, e.g.
os.RTLD_LAZY).        setprofile(function)

Set the profiling function.  It will be called on each function call
and return.  See the profiler chapter in the library manual.         getprofile($module, /)
--

Return the profiling function set with sys.setprofile.

See the profiler chapter in the library manual.              setrecursionlimit($module, limit, /)
--

Set the maximum depth of the Python interpreter stack to n.

This limit prevents infinite recursion from causing an overflow of the C
stack and crashing Python.  The highest possible limit is platform-
dependent.   settrace(function)

Set the global debug tracing function.  It will be called on each
function call.  See the debugger chapter in the library manual.           gettrace($module, /)
--

Return the global debug tracing function set with sys.settrace.

See the debugger chapter in the library manual.       call_tracing($module, func, args, /)
--

Call func(*args), while tracing is enabled.

The tracing state is saved, and restored afterwards.  This is intended
to