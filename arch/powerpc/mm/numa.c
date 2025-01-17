 the size of the buffer
  is chosen using a heuristic trying to determine the underlying device's
  "block size" and falling back on `io.DEFAULT_BUFFER_SIZE`.
  On many systems, the buffer will typically be 4096 or 8192 bytes long.

* "Interactive" text files (files for which isatty() returns True)
  use line buffering.  Other text files use the policy described above
  for binary files.

encoding is the name of the encoding used to decode or encode the
file. This should only be used in text mode. The default encoding is
platform dependent, but any encoding supported by Python can be
passed.  See the codecs module for the list of supported encodings.

errors is an optional string that specifies how encoding errors are to
be handled---this argument should not be used in binary mode. Pass
'strict' to raise a ValueError exception if there is an encoding error
(the default of None has the same effect), or pass 'ignore' to ignore
errors. (Note that ignoring encoding errors can lead to data loss.)
See the documentation for codecs.register or run 'help(codecs.Codec)'
for a list of the permitted encoding error strings.

newline controls how universal newlines works (it only applies to text
mode). It can be None, '', '\n', '\r', and '\r\n'.  It works as
follows:

* On input, if newline is None, universal newlines mode is
  enabled. Lines in the input can end in '\n', '\r', or '\r\n', and
  these are translated into '\n' before being returned to the
  caller. If it is '', universal newline mode is enabled, but line
  endings are returned to the caller untranslated. If it has any of
  the other legal values, input lines are only terminated by the given
  string, and the line ending is returned to the caller untranslated.

* On output, if newline is None, any '\n' characters written are
  translated to the system default line separator, os.linesep. If
  newline is '' or '\n', no translation takes place. If newline is any
  of the other legal values, any '\n' characters written are translated
  to the given string.

If closefd is False, the underlying file descriptor will be kept open
when the file is closed. This does not work when a file name is given
and must be True in that case.

A custom opener can be used by passing a callable as *opener*. The
underlying file descriptor for the file object is then obtained by
calling *opener* with (*file*, *flags*). *opener* must return an open
file descriptor (passing os.open as *opener* results in functionality
similar to passing None).

open() returns a file object whose type depends on the mode, and
through which the standard file operations such as reading and writing
are performed. When open() is used to open a file in a text mode ('w',
'r', 'wt', 'rt', etc.), it returns a TextIOWrapper. When used to open
a file in a binary mode, the returned class varies: in read binary
mode, it returns a BufferedReader; in write binary and append binary
modes, it returns a BufferedWriter, and in read/write mode, it returns
a BufferedRandom.

It is also possible to use a string or bytearray as a file for both
reading and writing. For strings StringIO can be used like a file
opened in a text mode, and for bytes a BytesIO can be used like a file
opened in a binary mode.               open_code($module, /, path)
--

Opens the provided file with the intent to import the contents.

This may perform extra validation beyond open(), but is otherwise interchangeable
with calling open(path, 'rb').               Base class for buffered IO objects.

The main difference with RawIOBase is that the read() method
supports omitting the size argument, and does not have a default
implementation that defers to readinto().

In addition, read(), readinto() and write() may raise
BlockingIOError if the underlying raw stream is in non-blocking
mode and not ready; unlike their raw counterparts, they will never
return None.

A typical implementation should not inherit from a RawIOBase
implementation, but wrap one.
                BufferedReader(raw, buffer_size=DEFAULT_BUFFER_SIZE)
--

Create a new buffered reader using the given readable raw IO object.   BufferedWriter(raw, buffer_size=DEFAULT_BUFFER_SIZE)
--

A buffer for a writeable sequential RawIO object.

The constructor creates a BufferedWriter for the given writeable raw
stream. If the buffer_size is not given, it defaults to
DEFAULT_BUFFER_SIZE.   BufferedRWPair(reader, writer, buffer_size=DEFAULT_BUFFER_SIZE, /)
--

A buffered reader and writer object together.

A buffered reader object and buffered writer object put together to
form a sequential IO object that can read and write. This is typically
used with a socket or two-way pipe.

reader and writer are RawIOBase objects that are readable and
writeable respectively. If the buffer_size is omitted it defaults to
DEFAULT_BUFFER_SIZE.   BufferedRandom(raw, buffer_size=DEFAULT_BUFFER_SIZE)
--

A buffered interface to random access streams.

The constructor creates a reader and writer for a seekable stream,
raw, given in the first argument. If the buffer_size is omitted it
defaults to DEFAULT_BUFFER_SIZE. detach($self, /)
--

Disconnect this buffer from its underlying raw stream and return it.

After the raw stream has been detached, the buffer is in an unusable
state.          Read and return up to n bytes.

If the argument is omitted, None, or negative, reads and
returns all data until EOF.

If the argument is positive, and the underlying raw stream is
not 'interactive', multiple raw reads may be issued to satisfy
the byte count (unless EOF is reached first).  But for
interactive raw streams (as well as sockets and pipes), at most
one raw read will be issued, and a short result does not imply
that EOF is imminent.

Returns an empty bytes object on EOF.

Returns None if the underlying raw stream was open in non-blocking
mode and no data is available at the moment.
         Read and return up to n bytes, with at most one read() call
to the underlying raw stream. A short result does not imply
that EOF is imminent.

Returns an empty bytes object on EOF.
           readinto($self, buffer, /)
--

 readinto1($self, buffer, /)
--

                Write the given buffer to the IO stream.

Returns the number of bytes written, which is always the length of b
in bytes.

Raises BlockingIOError if the buffer is full and the
underlying raw stream cannot accept more data at the moment.
    read($self, size=-1, /)
--

    peek($self, size=0, /)
--

     read1($self, size=-1, /)
--

   readinto($self, buffer, /)
--

 readinto1($self, buffer, /)
--

                readline($self, size=-1, /)
--

                seek($self, target, whence=0, /)
--

           truncate($self, pos=None, /)
--

               write($self, buffer, /)
--

    #hA  hA ojA ojA ojA ojA ojA ojA ojA ojA ojA ojA ojA ojA ojA ojA ojA XhA ojA ojA ojA ojA ghA �gA FileIO(file, mode='r', closefd=True, opener=None)
--

Open a file.

The mode can be 'r' (default), 'w', 'x' or 'a' for reading,
writing, exclusive creation or appending.  The file will be created if it
doesn't exist when opened for writing or appending; it will be truncated
when opened for writing.  A FileExistsError will be raised if it already
exists when opened for creating. Opening a file for creating implies
writing so this mode behaves in a similar way to 'w'.Add a '+' to the mode
to allow simultaneous reading and writing. A custom opener can be used by
passing a callable as *opener*. The underlying file descriptor for the file
object is then obtained by calling opener with (*name*, *flags*).
*opener* must return an open file descriptor (passing os.open as *opener*
results in functionality similar to passing None).                read($self, size=-1, /)
--

Read at most size bytes, returned as bytes.

Only makes one system call, so less data may be returned than requested.
In non-blocking mode, returns None if no data is available.
Return an empty bytes object at EOF.              readall($self, /)
--

Read all data from the file, returned as bytes.

In non-blocking mode, returns as much as is immediately available,
or None if no data is available.  Return an empty bytes object at EOF.                readinto($self, buffer, /)
--

Same as RawIOBase.readinto().    write($self, b, /)
--

Write buffer b to file, return number of bytes written.

Only makes one system call, so not all of the data may be written.
The number of bytes actually written is returned.  In non-blocking mode,
returns None if the write would block.              seek($self, pos, whence=0, /)
--

Move to new file position and return the file position.

Argument offset is a byte count.  Optional argument whence defaults to
SEEK_SET or 0 (offset from start of file, offset should be >= 0); other values
are SEEK_CUR or 1 (move relative to current position, positive or negative),
and SEEK_END or 2 (move relative to end of file, usually negative, although
many platforms allow seeking beyond the end of a file).

Note that not all file objects are seekable. tell($self, /)
--

Current file position.

Can raise OSError for non seekable files.            truncate($self, size=None, /)
--

Truncate the file to at most size bytes and return the truncated size.

Size defaults to the current file position, as returned by tell().
The current file position is changed to the value of size.         close($self, /)
--

Close the file.

A closed file cannot be used for further I/O operations.  close() may be
called more than once without error.              seekable($self, /)
--

True if file supports random-access.     readable($self, /)
--

True if file was opened in a read mode.  writable($self, /)
--

True if file was opened in a write mode. fileno($self, /)
--

Return the underlying file descriptor (an integer).        isatty($self, /)
--

True if the file is connected to a TTY device.             The abstract base class for all I/O classes, acting on streams of
bytes. There is no public constructor.

This class provides dummy implementations for many methods that
derived classes can override selectively; the default implementations
represent a file that cannot be read, written or seeked.

Even though IOBase does not declare read, readinto, or write because
their signatures will vary, implementations and clients should
consider those methods part of the interface. Also, implementations
may raise UnsupportedOperation when operations they do not support are
called.

The basic type used for binary data read from or written to a file is
bytes. Other bytes-like objects are accepted as method arguments too.
In some cases (such as readinto), a writable object is required. Text
I/O classes work with str data.

Note that calling any method (except additional calls to close(),
which are ignored) on a closed stream should raise a ValueError.

IOBase (and its subclasses) support the iterator protocol, meaning
that an IOBase object can be iterated over yielding the lines in a
stream.

IOBase also supports the :keyword:`with` statement. In this example,
fp is closed after the suite of the with statement is complete:

with open('spam.txt', 'r') as fp:
    fp.write('Spam and eggs!')
 Base class for raw binary I/O.  Change stream position.

Change the stream position to the given byte offset. The offset is
interpreted relative to the position indicated by whence.  Values
for whence are:

* 0 -- start of stream (the default); offset should be zero or positive
* 1 -- current stream position; offset may be negative
* 2 -- end of stream; offset is usually negative

Return the new absolute position.               tell($self, /)
--

Return current stream position.              Truncate file to size bytes.

File pointer is left unchanged.  Size defaults to the current IO
position as reported by tell().  Returns the new size.           flush($self, /)
--

Flush write buffers, if applicable.

This is not implemented for read-only and non-blocking streams.        close($self, /)
--

Flush and close the IO object.

This method has no effect if the file is already closed.    seekable($self, /)
--

Return whether object supports random access.

If False, seek(), tell() and truncate() will raise OSError.
This method may need to do a test seek().     readable($self, /)
--

Return whether object was opened for reading.

If False, read() will raise OSError.      writable($self, /)
--

Return whether object was opened for writing.

If False, write() will raise OSError.     fileno($self, /)
--

Returns underlying file descriptor if one exists.

OSError is raised if the IO object does not use a file descriptor.      isatty($self, /)
--

Return whether this is an 'interactive' stream.

Return False if it can't be determined.   readline($self, size=-1, /)
--

Read and return a line from the stream.

If size is specified, at most size bytes will be read.

The line terminator is always b'\n' for binary files; for text
files, the newlines argument to open can be used to select the line
terminator(s) recognized.   readlines($self, hint=-1, /)
--

Return a list of lines from the stream.

hint can be specified to control the number of lines read: no more
lines will be read if the total size (in bytes/characters) of all
lines so far exceeds hint.       writelines($self, lines, /)
--

Write a list of lines to stream.

Line separators are not added, so it is usual for each of the
lines provided to have a line separator at the end.             read($self, size=-1, /)
--

    readall($self, /)
--

Read until EOF, using multiple read() call.               BytesIO(initial_bytes=b'')
--

Buffered I/O implementation using an in-memory bytes buffer.     readable($self, /)
--

Returns True if the IO object can be read.               seekable($self, /)
--

Returns True if the IO object can be seeked.             writable($self, /)
--

Returns True if the IO object can be written.            close($self, /)
--

Disable all I/O operations. flush($self, /)
--

Does nothing.               isatty($self, /)
--

Always returns False.

BytesIO objects are not connected to a TTY-like device.             tell($self, /)
--

Current file position, an integer.           write($self, b, /)
--

Write bytes to file.

Return the number of bytes written.                writelines($self, lines, /)
--

Write lines to the file.

Note that newlines are not added.  lines can be any iterable object
producing bytes-like objects. This is equivalent to calling write() for
each element.             read1($self, size=-1, /)
--

Read at most size bytes, returned as a bytes object.

If the size argument is negative or omitted, read until EOF is reached.
Return an empty bytes object at EOF. readinto($self, buffer, /)
--

Read bytes into buffer.

Returns number of bytes read (0 for EOF), or None if the object
is set not to block and has no data to read.            readline($self, size=-1, /)
--

Next line from the file, as a bytes object.

Retain newline.  A non-negative size argument limits the maximum
number of bytes to return (an incomplete line may be returned then).
Return an empty bytes object at EOF.         readlines($self, size=None, /)
--

List of bytes objects, each a line from the file.

Call readline() repeatedly and return a list of the lines so read.
The optional size argument, if given, is an approximate bound on the
total number of bytes in the lines returned.      read($self, size=-1, /)
--

Read at most size bytes, returned as a bytes object.

If the size argument is negative, read until EOF is reached.
Return an empty bytes object at EOF.             getbuffer($self, /)
--

Get a read-write view over the contents of the BytesIO object.          getvalue($self, /)
--

Retrieve the entire contents of the BytesIO object.      seek($self, pos, whence=0, /)
--

Change stream position.

Seek to byte offset pos relative to position indicated by whence:
     0  Start of stream (the default).  pos should be >= 0;
     1  Current position - pos may be negative;
     2  End of stream - pos usually negative.
Returns the new absolute position.       truncate($self, size=None, /)
--

Truncate the file to at most size bytes.

Size defaults to the current file position, as returned by tell().
The current file position is unchanged.  Returns the new size.   �A )�A 5�A Q�A ]�A y�A ��A     Base class for text I/O.

This class provides a character and line based interface to stream
I/O. There is no readinto method because Python's character strings
are immutable. There is no public constructor.
                IncrementalNewlineDecoder(decoder, translate, errors='strict')
--

Codec used when reading a file in universal newlines mode.

It wraps another incremental decoder, translating \r\n and \r into \n.
It also records the types of newlines encountered.  When used with
translate=False, it ensures that the newline sequence is returned in
one piece. When used with decoder=None, it expects unicode strings as
decode input and translates newlines without first invoking an external
decoder.            TextIOWrapper(buffer, encoding=None, errors=None, newline=None,
              line_buffering=False, write_through=False)
--

Character and line based layer over a BufferedIOBase object, buffer.

encoding gives the name of the encoding that the stream will be
decoded or encoded with. It defaults to locale.getpreferredencoding(False).

errors determines the strictness of encoding and decoding (see
help(codecs.Codec) or the documentation for codecs.register) and
defaults to "strict".

newline controls how line endings are handled. It can be None, '',
'\n', '\r', and '\r\n'.  It works as follows:

* On input, if newline is None, universal newlines mode is
  enabled. Lines in the input can end in '\n', '\r', or '\r\n', and
  these are translated into '\n' before being returned to the
  caller. If it is '', universal newline mode is enabled, but line
  endings are returned to the caller untranslated. If it has any of
  the other legal values, input lines are only terminated by the given
  string, and the line ending is returned to the caller untranslated.

* On output, if newline is None, any '\n' characters written are
  translated to the system default line separator, os.linesep. If
  newline is '' or '\n', no translation takes place. If newline is any
  of the other legal values, any '\n' characters written are translated
  to the given string.

If line_buffering is True, a call to flush is implied when a call to
write contains a newline character.   Separate the underlying buffer from the TextIOBase and return it.

After the underlying buffer has been detached, the TextIO is in an
unusable state.
          Read at most n characters from stream.

Read from underlying buffer until we have n characters or we hit EOF.
If n is negative or omitted, read until EOF.
     Read until newline or EOF.

Returns an empty string if EOF is hit immediately.
 Write string to stream.
Returns the number of characters written (which is always equal to
the length of the string).
          Encoding of the text stream.

Subclasses should override.
      Line endings translated so far.

Only line endings translated during reading are considered.

Subclasses should override.
      The error setting of the decoder or encoder.

Subclasses should override.
      decode($self, /, input, final=False)
--

       getstate($self, /)
--

         setstate($self, state, /)
--

  reset($self, /)
--

            detach($self, /)
--

           reconfigure($self, /, *, encoding=None, errors=None, newline=None,
            line_buffering=None, write_through=None)
--

Reconfigure the text stream with new parameters.

This also does an implicit stream flush.          write($self, text, /)
--

      read($self, size=-1, /)
--

    readline($self, size=-1, /)
--

                flush($self, /)
--

            close($self, /)
--

            fileno($self, /)
--

           seekable($self, /)
--

         readable($self, /)
--

         writable($self, /)
--

         isatty($self, /)
--

           seek($self, cookie, whence=0, /)
--

           tell($self, /)
--

             truncate($self, pos=None, /)
--

               StringIO(initial_value='', newline='\n')
--

Text I/O implementation using an in-memory buffer.

The initial_value argument sets the value of object.  The newline
argument is like the one of TextIOWrapper's constructor.     close($self, /)
--

Close the IO object.

Attempting any further operation after the object is closed
will raise a ValueError.

This method has no effect if the file is already closed.        getvalue($self, /)
--

Retrieve the entire contents of the object.              read($self, size=-1, /)
--

Read at most size characters, returned as a string.

If the argument is negative or omitted, read until EOF
is reached. Return an empty string at EOF.              readline($self, size=-1, /)
--

Read until newline or EOF.

Returns an empty string if EOF is hit immediately.  tell($self, /)
--

Tell the current file position.              truncate($self, pos=None, /)
--

Truncate size to pos.

The pos argument defaults to the current file position, as
returned by tell().  The current file position is unchanged.
Returns the new absolute position.              seek($self, pos, whence=0, /)
--

Change stream position.

Seek to character offset pos relative to position indicated by whence:
    0  Start of stream (the default).  pos should be >= 0;
    1  Current position - pos must be 0;
    2  End of stream - pos must be 0.
Returns the new absolute position.  write($self, s, /)
--

Write string to file.

Returns the number of characters written, which is always equal to
the length of the string.      seekable($self, /)
--

Returns True if the IO object can be seeked.             readable($self, /)
--

Returns True if the IO object can be read.               writable($self, /)
--

Returns True if the IO object can be written.            �B tB �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B B �B B 'B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B 3B CB B �B YB �B �B YB YB YB YB YB YB YB YB YB YB YB YB YB YB YB YB YB YB YB YB �B B �B fB 3B uB ~B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B 3B �B �
B B B B B B B B B B B B B �
B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B B �
B B B B B B n
B B B B SB B B B B B B B �B B B B ]B B �B gB �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B VB �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B �B +B �B �B �B �B �B bB �B �B �B �B �B �B �B json speedups
          encode_basestring_ascii(string) -> string

Return an ASCII-only JSON representation of a Python string          encode_basestring(string) -> string

Return a JSON representation of a Python string            scanstring(string, end, strict=True) -> (string, end)

Scan the string s for a JSON string. End is the index of the
character in s after the quote that started the JSON string.
Unescapes all valid JSON string escape sequences and raises ValueError
on attempt to decode an invalid string. If strict is False then literal
control characters are allowed in the string.

Returns a tuple of the decoded string and the index of the character in s
after the end quote.   JSON scanner object             _iterencode(obj, _current_indent_level) -> iterable                                                                           	                               
                                                                                                                          
                  ����������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������Support for POSIX locales.      (integer,string=None) -> string. Activates/queries locale processing.           () -> dict. Returns numeric and monetary locale-specific parameters.            string,string -> int. Compares two strings according to the locale.             strxfrm(string) -> string.

Return a string that can be used as a key for locale-aware comparisons.             nl_langinfo(key) -> string
Return the value for the locale information associated with key.     gettext(msg) -> string
Return translation of msg.               dgettext(domain, msg) -> string
Return translation of msg in domain.            dcgettext(domain, msg, category) -> string
Return translation of msg in domain and category.    textdomain(domain) -> string
Set the C library's textdmain to domain, returning the new domain. bindtextdomain(domain, dir) -> string
Bind the C library's domain to dir.       bind_textdomain_codeset(domain, codeset) -> string
Bind the C library's domain to codeset.      YB �YB �YB >YB uYB �XB �XB     Profiler(timer=None, timeunit=None, subcalls=True, builtins=True)

    Builds a profiler object using the specified timer function.
    The default timer is a fast built-in one based on real time.
    For custom timer functions returning integers, timeunit can
    be a float specifying a scale (i.e. how long each integer unit
    is, in seconds).
   getstats() -> list of profiler_entry objects

Return all information collected by the profiler.
Each profiler_entry is a tuple-like object with the
following attributes:

    code          code object
    callcount     how many times this was called
    reccallcount  how many times called recursively
    totaltime     total time in this entry
    inlinetime    inline time in this entry (not in subcalls)
    calls         details of the calls

The calls attribute is either None or a list of
profiler_subentry objects:

    code          called code object
    callcount     how many times this is called
    reccallcount  how many times this is called recursively
    totaltime     total time spent in this call
    inlinetime    inline time (not in further subcalls)
            enable(subcalls=True, builtins=True)

Start collecting profiling information.
If 'subcalls' is True, also records for each function
statistics separated according to its current caller.
If 'builtins' is True, records the time spent in
built-in functions separately from their caller.
    disable()

Stop collecting profiling information.
              clear()

Clear all profiling information collected so far.
     copy($self, /)
--

Return a copy of the hash object.            digest($self, /)
--

Return the digest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              update($self, obj, /)
--

Update this hash object's state with the provided string.             md5($module, /, string=b'', *, usedforsecurity=True)
--

Return a new MD5 hash object; optionally initialized with a string.    ��B N�B |�B ��B encode($self, /, input, final=False)
--

       getstate($self, /)
--

         setstate($self, state, /)
--

  reset($self, /)
--

            decode($self, /, input, final=False)
--

       getstate($self, /)
--

         setstate($self, state, /)
--

  reset($self, /)
--

            read($self, sizeobj=None, /)
--

               readline($self, sizeobj=None, /)
--

           readlines($self, sizehintobj=None, /)
--

      reset($self, /)
--

            write($self, strobj, /)
--

    writelines($self, lines, /)
--

                reset($self, /)
--

            encode($self, /, input, errors=None)
--

Return an encoded string version of `input'.

'errors' may be given to set a different error handling scheme. Default is
'strict' meaning that encoding errors raise a UnicodeEncodeError. Other possible
values are 'ignore', 'replace' and 'xmlcharrefreplace' as well as any other name
registered with codecs.register_error that can handle UnicodeEncodeErrors.  decode($self, /, input, errors=None)
--

Decodes 'input'.

'errors' may be given to set a different error handling scheme. Default is
'strict' meaning that encoding errors raise a UnicodeDecodeError. Other possible
values are 'ignore' and 'replace' as well as any other name registered with
codecs.register_error that is able to handle UnicodeDecodeErrors."           __create_codec($module, arg, /)
--

            stack_effect($module, opcode, oparg=None, /, *, jump=None)
--

Compute the stack effect of the opcode.          Operator interface.

This module exports a set of functions implemented in C corresponding
to the intrinsic operators of Python.  For example, operator.add(x, y)
is equivalent to the expression x+y.  The function names are those
used for special methods; variants without leading and trailing
'__' are also provided for convenience.    truth($module, a, /)
--

Return True if a is true, False otherwise.             contains($module, a, b, /)
--

Same as b in a (note reversed operands).         indexOf($module, a, b, /)
--

Return the first index of b in a. countOf($module, a, b, /)
--

Return the number of times b occurs in a.         is_($module, a, b, /)
--

Same as a is b.       is_not($module, a, b, /)
--

Same as a is not b.                index($module, a, /)
--

Same as a.__index__()  add($module, a, b, /)
--

Same as a + b.        sub($module, a, b, /)
--

Same as a - b.        mul($module, a, b, /)
--

Same as a * b.        matmul($module, a, b, /)
--

Same as a @ b.     floordiv($module, a, b, /)
--

Same as a // b.  truediv($module, a, b, /)
--

Same as a / b.    mod($module, a, b, /)
--

Same as a % b.        neg($module, a, /)
--

Same as -a.              pos($module, a, /)
--

Same as +a.              abs($module, a, /)
--

Same as abs(a).          inv($module, a, /)
--

Same as ~a.              invert($module, a, /)
--

Same as ~a.           lshift($module, a, b, /)
--

Same as a << b.    rshift($module, a, b, /)
--

Same as a >> b.    not_($module, a, /)
--

Same as not a.          and_($module, a, b, /)
--

Same as a & b.       xor($module, a, b, /)
--

Same as a ^ b.        or_($module, a, b, /)
--

Same as a | b.        iadd($module, a, b, /)
--

Same as a += b.      isub($module, a, b, /)
--

Same as a -= b.      imul($module, a, b, /)
--

Same as a *= b.      imatmul($module, a, b, /)
--

Same as a @= b.   ifloordiv($module, a, b, /)
--

Same as a //= b.                itruediv($module, a, b, /)
--

Same as a /= b.  imod($module, a, b, /)
--

Same as a %= b.      ilshift($module, a, b, /)
--

Same as a <<= b.  irshift($module, a, b, /)
--

Same as a >>= b.  iand($module, a, b, /)
--

Same as a &= b.      ixor($module, a, b, /)
--

Same as a ^= b.      ior($module, a, b, /)
--

Same as a |= b.       concat($module, a, b, /)
--

Same as a + b, for a and b sequences.              iconcat($module, a, b, /)
--

Same as a += b, for a and b sequences.            getitem($module, a, b, /)
--

Same as a[b].     setitem($module, a, b, c, /)
--

Same as a[b] = c.              delitem($module, a, b, /)
--

Same as del a[b]. pow($module, a, b, /)
--

Same as a ** b.       ipow($module, a, b, /)
--

Same as a **= b.     eq($module, a, b, /)
--

Same as a == b.        ne($module, a, b, /)
--

Same as a != b.        lt($module, a, b, /)
--

Same as a < b.         le($module, a, b, /)
--

Same as a <= b.        gt($module, a, b, /)
--

Same as a > b.         ge($module, a, b, /)
--

Same as a >= b.        _compare_digest($module, a, b, /)
--

Return 'a == b'.

This function uses an approach designed to prevent
timing analysis, making it appropriate for cryptography.

a and b must both be of the same type: either str (ASCII only),
or any bytes-like object.

Note: If a and b are of different lengths, or if an error occurs,
a timing attack could theoretically reveal information about the
types and lengths of a and b--but not their values.          length_hint($module, obj, default=0, /)
--

Return an estimate of the number of items in obj.

This is useful for presizing containers when building from an iterable.

If the object supports len(), the result will be exact.
Otherwise, it may over- or under-estimate by an arbitrary amount.
The result will be an integer >= 0.           itemgetter(item, ...) --> itemgetter object

Return a callable object that fetches the given item(s) from its operand.
After f = itemgetter(2), the call f(r) returns r[2].
After g = itemgetter(2, 5, 3), the call g(r) returns (r[2], r[5], r[3])             Return state information for pickling           attrgetter(attr, ...) --> attrgetter object

Return a callable object that fetches the given attribute(s) from its operand.
After f = attrgetter('name'), the call f(r) returns r.name.
After g = attrgetter('name', 'date'), the call g(r) returns (r.name, r.date).
After h = attrgetter('name.first', 'name.last'), the call h(r) returns
(r.name.first, r.name.last).       methodcaller(name, ...) --> methodcaller object

Return a callable object that calls the given method on its operand.
After f = methodcaller('name'), the call f(r) returns r.name().
After g = methodcaller('name', 'date', foo=1), the call g(r) returns
r.name('date', foo=1).               �FC PC OC �QC �RC �QC QC 0SC TC �QC �RC mTC  TC hQC �TC �LC �TC �LC �MC .UC TC �UC �JC VC DXC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC ZMC 2TC �wC �wC �wC �wC |C �wC �YC ~WC [UC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC �wC :VC LVC �wC �wC �HC kIC �wC -OC yZC �YC WRC �KC �IC �wC ^VC �IC �VC �JC lWC 3FC ZC �wC �TC �wC �wC �wC �wC HC �wC �wC �wC zQC �XC JC �WC �RC �wC �KC �YC GC �NC �wC �GC �wC �wC �ZC /HC �ZC �GC [C �UC �OC �wC �wC �wC �wC �wC �wC �wC �GC         Optimized C implementation for the Python pickle module.        dump($module, /, obj, file, protocol=None, *, fix_imports=True,
     buffer_callback=None)
--

Write a pickled representation of obj to the open file object file.

This is equivalent to ``Pickler(file, protocol).dump(obj)``, but may
be more efficient.

The optional *protocol* argument tells the pickler to use the given
protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default
protocol is 4. It was introduced in Python 3.4, and is incompatible
with previous versions.

Specifying a negative protocol version selects the highest protocol
version supported.  The higher the protocol used, the more recent the
version of Python needed to read the pickle produced.

The *file* argument must have a write() method that accepts a single
bytes argument.  It can thus be a file object opened for binary
writing, an io.BytesIO instance, or any other custom object that meets
this interface.

If *fix_imports* is True and protocol is less than 3, pickle will try
to map the new Python 3 names to the old module names used in Python
2, so that the pickle data stream is readable with Python 2.

If *buffer_callback* is None (the default), buffer views are serialized
into *file* as part of the pickle stream.  It is an error if
*buffer_callback* is not None and *protocol* is None or smaller than 5.           dumps($module, /, obj, protocol=None, *, fix_imports=True,
      buffer_callback=None)
--

Return the pickled representation of the object as a bytes object.

The optional *protocol* argument tells the pickler to use the given
protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default
protocol is 4. It was introduced in Python 3.4, and is incompatible
with previous versions.

Specifying a negative protocol version selects the highest protocol
version supported.  The higher the protocol used, the more recent the
version of Python needed to read the pickle produced.

If *fix_imports* is True and *protocol* is less than 3, pickle will
try to map the new Python 3 names to the old module names used in
Python 2, so that the pickle data stream is readable with Python 2.

If *buffer_callback* is None (the default), buffer views are serialized
into *file* as part of the pickle stream.  It is an error if
*buffer_callback* is not None and *protocol* is None or smaller than 5.    load($module, /, file, *, fix_imports=True, encoding='ASCII',
     errors='strict', buffers=())
--

Read and return an object from the pickle data stored in a file.

This is equivalent to ``Unpickler(file).load()``, but may be more
efficient.

The protocol version of the pickle is detected automatically, so no
protocol argument is needed.  Bytes past the pickled object's
representation are ignored.

The argument *file* must have two methods, a read() method that takes
an integer argument, and a readline() method that requires no
arguments.  Both methods should return bytes.  Thus *file* can be a
binary file object opened for reading, an io.BytesIO object, or any
other custom object that meets this interface.

Optional keyword arguments are *fix_imports*, *encoding* and *errors*,
which are used to control compatibility support for pickle stream
generated by Python 2.  If *fix_imports* is True, pickle will try to
map the old Python 2 names to the new names used in Python 3.  The
*encoding* and *errors* tell pickle how to decode 8-bit string
instances pickled by Python 2; these default to 'ASCII' and 'strict',
respectively.  The *encoding* can be 'bytes' to read these 8-bit
string instances as bytes objects.        loads($module, data, /, *, fix_imports=True, encoding='ASCII',
      errors='strict', buffers=())
--

Read and return an object from the given pickle data.

The protocol version of the pickle is detected automatically, so no
protocol argument is needed.  Bytes past the pickled object's
representation are ignored.

Optional keyword arguments are *fix_imports*, *encoding* and *errors*,
which are used to control compatibility support for pickle stream
generated by Python 2.  If *fix_imports* is True, pickle will try to
map the old Python 2 names to the new names used in Python 3.  The
*encoding* and *errors* tell pickle how to decode 8-bit string
instances pickled by Python 2; these default to 'ASCII' and 'strict',
respectively.  The *encoding* can be 'bytes' to read these 8-bit
string instances as bytes objects.           clear($self, /)
--

Remove all items from memo. copy($self, /)
--

Copy the memo to a new object.               __reduce__($self, /)
--

Implement pickle support.              clear($self, /)
--

Remove all items from memo. copy($self, /)
--

Copy the memo to a new object.               __reduce__($self, /)
--

Implement pickling support.            Pickler(file, protocol=None, fix_imports=True, buffer_callback=None)
--

This takes a binary file for writing a pickle data stream.

The optional *protocol* argument tells the pickler to use the given
protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default
protocol is 4. It was introduced in Python 3.4, and is incompatible
with previous versions.

Specifying a negative protocol version selects the highest protocol
version supported.  The higher the protocol used, the more recent the
version of Python needed to read the pickle produced.

The *file* argument must have a write() method that accepts a single
bytes argument. It can thus be a file object opened for binary
writing, an io.BytesIO instance, or any other custom object that meets
this interface.

If *fix_imports* is True and protocol is less than 3, pickle will try
to map the new Python 3 names to the old module names used in Python
2, so that the pickle data stream is readable with Python 2.

If *buffer_callback* is None (the default), buffer views are
serialized into *file* as part of the pickle stream.

If *buffer_callback* is not None, then it can be called any number
of times with a buffer view.  If the callback returns a false value
(such as None), the given buffer is out-of-band; otherwise the
buffer is serialized in-band, i.e. inside the pickle stream.

It is an error if *buffer_callback* is not None and *protocol*
is None or smaller than 5.                dump($self, obj, /)
--

Write a pickled representation of the given object to the open file.    clear_memo($self, /)
--

Clears the pickler's "memo".

The memo is the data structure that remembers which objects the
pickler has already seen, so that shared or recursive objects are
pickled by reference and not by value.  This method is useful when
re-using picklers.  __sizeof__($self, /)
--

Returns size in memory, in bytes.      Unpickler(file, *, fix_imports=True, encoding='ASCII', errors='strict',
          buffers=())
--

This takes a binary file for reading a pick