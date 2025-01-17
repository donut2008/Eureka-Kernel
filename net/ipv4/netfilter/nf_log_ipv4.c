d by new.

  count
    Maximum number of occurrences to replace.
    -1 (the default value) means replace all occurrences.

If the optional argument count is given, only the first count occurrences are
replaced.        removeprefix($self, prefix, /)
--

Return a bytearray with the given prefix string removed if present.

If the bytearray starts with the prefix string, return
bytearray[len(prefix):].  Otherwise, return a copy of the original
bytearray.    removesuffix($self, suffix, /)
--

Return a bytearray with the given suffix string removed if present.

If the bytearray ends with the suffix string and that suffix is not
empty, return bytearray[:-len(suffix)].  Otherwise, return a copy of
the original bytearray.        reverse($self, /)
--

Reverse the order of the values in B in place.            rjust($self, width, fillchar=b' ', /)
--

Return a right-justified string of length width.

Padding is done using the specified fill character. rpartition($self, sep, /)
--

Partition the bytearray into three parts using the given separator.

This will search for the separator sep in the bytearray, starting at the end.
If the separator is found, returns a 3-tuple containing the part before the
separator, the separator itself, and the part after it as new bytearray
objects.

If the separator is not found, returns a 3-tuple containing two empty bytearray
objects and the copy of the original bytearray object.           rsplit($self, /, sep=None, maxsplit=-1)
--

Return a list of the sections in the bytearray, using sep as the delimiter.

  sep
    The delimiter according which to split the bytearray.
    None (the default value) means split on ASCII whitespace characters
    (space, tab, return, newline, formfeed, vertical tab).
  maxsplit
    Maximum number of splits to do.
    -1 (the default value) means no limit.

Splitting is done starting at the end of the bytearray and working to the front.         rstrip($self, bytes=None, /)
--

Strip trailing bytes contained in the argument.

If the argument is omitted or None, strip trailing ASCII whitespace.          split($self, /, sep=None, maxsplit=-1)
--

Return a list of the sections in the bytearray, using sep as the delimiter.

  sep
    The delimiter according which to split the bytearray.
    None (the default value) means split on ASCII whitespace characters
    (space, tab, return, newline, formfeed, vertical tab).
  maxsplit
    Maximum number of splits to do.
    -1 (the default value) means no limit.            splitlines($self, /, keepends=False)
--

Return a list of the lines in the bytearray, breaking at line boundaries.

Line breaks are not included in the resulting list unless keepends is given and
true.       strip($self, bytes=None, /)
--

Strip leading and trailing bytes contained in the argument.

If the argument is omitted or None, strip leading and trailing ASCII whitespace.   translate($self, table, /, delete=b'')
--

Return a copy with each character mapped by the given translation table.

  table
    Translation table, which must be a bytes object of length 256.

All characters occurring in the optional argument delete are removed.
The remaining characters are mapped through the given translation table. zfill($self, width, /)
--

Pad a numeric string with zeros on the left, to fill a field of the given width.

The original string is never truncated.            Private method returning an estimate of len(list(it)).          Set state information for unpickling.           bool(x) -> bool

Returns True when the argument x is true, False otherwise.
The builtins True and False are the only two instances of the class bool.
The class bool is a subclass of the class int, and cannot be subclassed.  �=6 �=6 �=6 �=6 �=6 �=6         int([x]) -> integer
int(x, base=10) -> integer

Convert a number or string to an integer, or return 0 if no arguments
are given.  If x is a number, return x.__int__().  For floating point
numbers, this truncates towards zero.

If x is not a number or if base is given, then x must be a string,
bytes, or bytearray instance representing an integer literal in the
given base.  The literal can be preceded by '+' or '-' and be surrounded
by whitespace.  The base defaults to 10.  Valid bases are 0 and 2-36.
Base 0 means to interpret the base from the string as an integer literal.
>>> int('0b100', base=0)
4   bit_length($self, /)
--

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6  to_bytes($self, /, length, byteorder, *, signed=False)
--

Return an array of bytes representing an integer.

  length
    Length of bytes object to use.  An OverflowError is raised if the
    integer is not representable with the given number of bytes.
  byteorder
    The byte order used to represent the integer.  If byteorder is 'big',
    the most significant byte is at the beginning of the byte array.  If
    byteorder is 'little', the most significant byte is at the end of the
    byte array.  To request the native byte order of the host system, use
    `sys.byteorder' as the byte order value.
  signed
    Determines whether two's complement is used to represent the integer.
    If signed is False and a negative integer is given, an OverflowError
    is raised.        from_bytes($type, /, bytes, byteorder, *, signed=False)
--

Return the integer represented by the given array of bytes.

  bytes
    Holds the array of bytes to convert.  The argument must either
    support the buffer protocol or be an iterable object producing bytes.
    Bytes and bytearray are examples of built-in objects that support the
    buffer protocol.
  byteorder
    The byte order used to represent the integer.  If byteorder is 'big',
    the most significant byte is at the beginning of the byte array.  If
    byteorder is 'little', the most significant byte is at the end of the
    byte array.  To request the native byte order of the host system, use
    `sys.byteorder' as the byte order value.
  signed
    Indicates whether two's complement is used to represent the integer.  as_integer_ratio($self, /)
--

Return integer ratio.

Return a pair of integers, whose ratio is exactly equal to the original int
and with a positive denominator.

>>> (10).as_integer_ratio()
(10, 1)
>>> (-10).as_integer_ratio()
(-10, 1)
>>> (0).as_integer_ratio()
(0, 1) __getnewargs__($self, /)
--

   __format__($self, format_spec, /)
--

          __sizeof__($self, /)
--

Returns size in memory, in bytes.      sys.int_info

A named tuple that holds information about Python's
internal representation of integers.  The attributes are read only.           	
             ��124789ABCEFGHIJKLMN�������������12345679:;<=>?@A�BDEFGHJKLMN��%   H   :   %   M   :   %   S       ��������       ����      ��� ��	
������%   m   /   %   d   /   %   y   	
               ����    ����       %   Y   -   %   m   -   %   d   
           ���OPQRS��TUVWXY��Z[\]^_��`abc��g�	j��g�r�n<:�O�RQ�h��ك��[��	
���                        �� 	
�������������6 N�6 ~�6 ~�6 ~�6 ~�6 
�6 ~�6 ~�6 ~�6 ~�6 ~�6 ~�6 ,�6 ~�6 ~�6 ��6 ~�6 ϒ6 ~�6 ~�6 ��6 e�6 ��6 ��6 o�6 ��6 ��6 ��6 ��6 ��6 ��6 ʢ6 j�6 ��6 t�6 ��6 ��6 1�6 ��6 ��6 ��6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 R�6 �6 �6 �6 �6 �6 �6 �6 �6 d�6 ��6 ��6 R�6 ��6 ��6 ��6 �6 R�6 �6 �6 �6 �6 �6 R�6 �6 �6 d�6 ��6 �6 R�6 �6 �6 R�6 g�6 ��6 ��6 ��6 ��6 g�6 ��6 ��6 ��6 ��6 ��6 ��6 ��6 ��6 ��6 ��6 ��6 g�6 ��6 ��6 b�6 !�6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 2�6 �6 �6 �6 �6 ֲ6 �6 �6 �6 �6 �6 �6 �6 �6 d�6 d�6 d�6 d�6 d�6 d�6 d�6 d�6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 ޲6 �6 �6 �6 �6 β6 Ʋ6 �6 �6 �6 :�6 �6 �6 �6 �6 �6 �6 �6 B�6 �6 �6 �6 R�6 �6 J�6 �6 ��6 �6 �6 7�6 7�6 �6 �6 7�6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 �6 .�6 #�6 ��6 n�6 ��6 ��6 ��6 E�6 \�6 g�6 r�6 }�6 ��6         bytes(iterable_of_ints) -> bytes
bytes(string, encoding[, errors]) -> bytes
bytes(bytes_or_buffer) -> immutable copy of bytes_or_buffer
bytes(int) -> bytes object of size given by the parameter initialized with null bytes
bytes() -> empty bytes object

Construct an immutable array of bytes from:
  - an iterable yielding integers in range(256)
  - a text string encoded using the specified encoding
  - any object implementing the buffer API.
  - an integer      center($self, width, fillchar=b' ', /)
--

Return a centered string of length width.

Padding is done using the specified fill character.       decode($self, /, encoding='utf-8', errors='strict')
--

Decode the bytes using the codec registered for encoding.

  encoding
    The encoding with which to decode the bytes.
  errors
    The error handling scheme to use for the handling of decoding errors.
    The default is 'strict' meaning that decoding errors raise a
    UnicodeDecodeError. Other possible values are 'ignore' and 'replace'
    as well as any other name registered with codecs.register_error that
    can handle UnicodeDecodeErrors.        expandtabs($self, /, tabsize=8)
--

Return a copy where all tab characters are expanded using spaces.

If tabsize is not given, a tab size of 8 characters is assumed.          fromhex($type, string, /)
--

Create a bytes object from a string of hexadecimal numbers.

Spaces between two numbers are accepted.
Example: bytes.fromhex('B9 01EF') -> b'\\xb9\\x01\\xef'.    hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)
--

Create a str of hexadecimal numbers from a bytes object.

  sep
    An optional single character or byte to separate hex bytes.
  bytes_per_sep
    How many bytes between separators.  Positive values count from the
    right, negative values count from the left.

Example:
>>> value = b'\xb9\x01\xef'
>>> value.hex()
'b901ef'
>>> value.hex(':')
'b9:01:ef'
>>> value.hex(':', 2)
'b9:01ef'
>>> value.hex(':', -2)
'b901:ef'  join($self, iterable_of_bytes, /)
--

Concatenate any number of bytes objects.

The bytes whose method is called is inserted in between