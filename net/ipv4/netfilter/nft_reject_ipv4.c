 information for pickling. __reduce_ex__($self, proto=0, /)
--

Return state information for pickling.     __sizeof__($self, /)
--

Returns the size of the bytearray object in memory, in bytes.          append($self, item, /)
--

Append a single item to the end of the bytearray.

  item
    The item to be appended.               center($self, width, fillchar=b' ', /)
--

Return a centered string of length width.

Padding is done using the specified fill character.       clear($self, /)
--

Remove all items from the bytearray.        copy($self, /)
--

Return a copy of B.          decode($self, /, encoding='utf-8', errors='strict')
--

Decode the bytearray using the codec registered for encoding.

  encoding
    The encoding with which to decode the bytearray.
  errors
    The error handling scheme to use for the handling of decoding errors.
    The default is 'strict' meaning that decoding errors raise a
    UnicodeDecodeError. Other possible values are 'ignore' and 'replace'
    as well as any other name registered with codecs.register_error that
    can handle UnicodeDecodeErrors.                expandtabs($self, /, tabsize=8)
--

Return a copy where all tab characters are expanded using spaces.

If tabsize is not given, a tab size of 8 characters is assumed.          extend($self, iterable_of_ints, /)
--

Append all the items from the iterator or sequence to the end of the bytearray.

  iterable_of_ints
    The iterable of items to append. fromhex($type, string, /)
--

Create a bytearray object from a string of hexadecimal numbers.

Spaces between two numbers are accepted.
Example: bytearray.fromhex('B9 01EF') -> bytearray(b'\\xb9\\x01\\xef')  hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)
--

Create a str of hexadecimal numbers from a bytearray object.

  sep
    An optional single character or byte to separate hex bytes.
  bytes_per_sep
    How many bytes between separators.  Positive values count from the
    right, negative values count from the left.

Example:
>>> value = bytearray([0xb9, 0x0