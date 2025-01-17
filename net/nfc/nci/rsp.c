nd a task.

Task belongs to loop.

Returns None.           Future(*, loop=None)
--

This class is *almost* compatible with concurrent.futures.Future.

    Differences:

    - result() and exception() do not take a timeout argument and
      raise an exception when the future isn't done yet.

    - Callbacks registered with add_done_callback() are always called
      via the event loop's call_soon_threadsafe().

    - This class is not compatible with the wait() and as_completed()
      methods in the concurrent.futures package.      result($self, /)
--

Return the result this future represents.

If the future has been cancelled, raises CancelledError.  If the
future's result isn't yet available, raises InvalidStateError.  If
the future is done and has an exception set, this exception is raised.      exception($self, /)
--

Return the exception that was set on this future.

The exception (or None if no exception was set) is returned only if
the future is done.  If the future has been cancelled, raises
CancelledError.  If the future isn't done yet, raises
InvalidStateError.           set_result($self, result, /)
--

Mark the future done and set its result.

If the future is already done when this method is called, raises
InvalidStateError.  set_exception($self, exception, /)
--

Mark the future done and set an exception.

If the future is already done when this method is called, raises
InvalidStateError.          add_done_callback($self, fn, /, *, context=<unrepresentable>)
--

Add a callback to be run when the future becomes done.

The callback is called with a single argument - the future object. If
the future is already done when this is called, the callback is
scheduled with call_soon.       remove_done_callback($self, fn, /)
--

Remove all instances of a callback from the "call when done" list.

Returns the number of callbacks removed.             cancel($self, /, msg=None)
--

Cancel the future and schedule callbacks.

If the future is already done or cancelled, return False.  Otherwise,
change the future's state to cancelled, schedule the callbacks and
return True. cancelled($self, /)
--

Return True if the future was cancelled.                done($self, /)
--

Return True if the future is done.

Done means either that a result / exception are available, or that the
future was cancelled.             get_loop($self, /)
--

Return the event loop the Future is bound to.            _make_cancelled_error($self, /)
--

Create the CancelledError to raise if the Future is cancelled.

This should only be called once when handling a cancellation since
it erases the context exception value.   _repr_info($self, /)
--

       Task(coro, *, loop=None, name=None)
--

A coroutine wrapped in a Future.        set_result($self, result, /)
--

               set_exception($self, exception, /)
--

         cancel($self, /, msg=None)
--

Request that this task cancel itself.

This arranges for a CancelledError to be thrown into the
wrapped coroutine on the next cycle through the event loop.
The coroutine then has a chance to clean up or even deny
the request using try/except/finally.

Unlike Future.cancel, this does not guarantee that the
task will be cancelled: the exception might be caught and
acted upon, delaying cancellation of the task or preventing
cancellation completely.  The task may also return a value or
raise a different exception.

Immediately after this method is called, Task.cancelled() will
not return True (unless the task was already cancelled).  A
task will be marked as cancelled when the wrapped coroutine
terminates with a CancelledError exception (even if cancel()
was not called).        get_stack($self, /, *, limit=None)
--

Return the list of stack frames for this task's coroutine.

If the coroutine is not done, this returns the stack where it is
suspended.  If the coroutine has completed successfully or was
cancelled, this returns an empty list.  If the coroutine was
terminated by an exception, this returns the list of traceback
frames.

The frames are always ordered from oldest to newest.

The optional limit gives the maximum number of frames to
return; by default all available frames are returned.  Its
meaning differs depending on whether a stack or a traceback is
returned: the newest frames of a stack are returned, but the
oldest frames of a traceback are returned.  (This matches the
behavior of the traceback module.)

For reasons beyond our control, only one stack frame is
returned for a suspended coroutine.     print_stack($self, /, *, limit=None, file=None)
--

Print the stack or traceback for this task's coroutine.

This produces output similar to that of the traceback module,
for the frames retrieved by get_stack().  The limit argument
is passed to get_stack().  The file argument is an I/O stream
to which the output is written; by default output is written
to sys.stderr.               _make_cancelled_error($self, /)
--

Create the CancelledError to raise if the Task is cancelled.

This should only be called once when handling a cancellation since
it erases the context exception value.     _repr_info($self, /)
--

       get_name($self, /)
--

         set_name($self, value, /)
--

  get_coro($self, /)
--

         Bisection algorithms.

This module provides support for maintaining a list in sorted order without
having to sort the list after each insertion. For long lists of items with
expensive comparison operations, this can be an improvement over the more
common approach.
       bisect_right($module, /, a, x, lo=0, hi=None)
--

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e <= x, and all e in
a[i:] have e > x.  So if x already appears in the list, i points just
beyond the rightmost x already there

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.      insort_right($module, /, a, x, lo=0, hi=None)
--

Insert item x in list a, and keep it sorted assuming a is sorted.

If x is already in a, insert it to the right of the rightmost x.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.                bisect_left($module, /, a, x, lo=0, hi=None)
--

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e < x, and all e in
a[i:] have e >= x.  So if x already appears in the list, i points just
before the leftmost x already there.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.       insort_left($module, /, a, x, lo=0, hi=None)
--

Insert item x in list a, and keep it sorted assuming a is sorted.

If x is already in a, insert it to the left of the leftmost x.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.   _blake2b provides BLAKE2b for hashlib
          blake2b(data=b'', /, *, digest_size=_blake2.blake2b.MAX_DIGEST_SIZE,
        key=b'', salt=b'', person=b'', fanout=1, depth=1, leaf_size=0,
        node_offset=0, node_depth=0, inner_size=0, last_node=False,
        usedforsecurity=True)
--

Return a new BLAKE2b hash object.             ɼ�g�	j;�ʄ��g�+���r�n<�6_:�O�т�RQl>+�h�k�A��كy!~��[copy($self, /)
--

Return a copy of the hash object.            digest($self, /)
--

Return the digest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              update($self, data, /)
--

Update this hash object's state with the provided bytes-like object. blake2s(data=b'', /, *, digest_size=_blake2.blake2s.MAX_DIGEST_SIZE,
        key=b'', salt=b'', person=b'', fanout=1, depth=1, leaf_size=0,
        node_offset=0, node_depth=0, inner_size=0, last_node=False,
        usedforsecurity=True)
--

Return a new BLAKE2s hash object.             copy($self, /)
--

Return a copy of the hash object.            digest($self, /)
--

Return the digest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              update($self, data, /)
--

Update this hash object's state with the provided bytes-like object. �QG RG �QG �QG "RG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG     register($module, search_function, /)
--

Register a codec search function.

Search functions are expected to take one argument, the encoding name in
all lower case letters, and either return None, or a tuple of functions
(encoder, decoder, stream_reader, stream_writer) (or a CodecInfo object).         lookup($module, encoding, /)
--

Looks up a codec tuple in the Python codec registry and returns a CodecInfo object.            encode($module, /, obj, encoding='utf-8', errors='strict')
--

Encodes obj using the codec registered for encoding.

The default encoding is 'utf-8'.  errors may be given to set a
different error handling scheme.  Default is 'strict' meaning that encoding
errors raise a ValueError.  Other possible values are 'ignore', 'replace'
and 'backslashreplace' as well as any other name registered with
codecs.register_error that can handle ValueErrors.   decode($module, /, obj, encoding='utf-8', errors='strict')
--

Decodes obj using the codec registered for encoding.

Default encoding is 'utf-8'.  errors may be given to set a
different error handling scheme.  Default is 'strict' meaning that encoding
errors raise a ValueError.  Other possible values are 'ignore', 'replace'
and 'backslashreplace' as well as any other name registered with
codecs.register_error that can handle ValueErrors.       escape_encode($module, data, errors=None, /)
--

               escape_