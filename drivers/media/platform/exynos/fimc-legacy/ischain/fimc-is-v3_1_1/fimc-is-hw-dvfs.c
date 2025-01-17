tbuffer($self, /)
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

The optional *protocol* argument tells the pick