ght-justified string of length width.

Padding is done using the specified fill character. rpartition($self, sep, /)
--

Partition the bytes into three parts using the given separator.

This will search for the separator sep in the bytes, starting at the end. If
the separator is found, returns a 3-tuple containing the part before the
separator, the separator itself, and the part after it.

If the separator is not found, returns a 3-tuple containing two empty bytes
objects and the original bytes object.                rsplit($self, /, sep=None, maxsplit=-1)
--

Return a list of the sections in the bytes, using sep as the delimiter.

  sep
    The delimiter according which to split the bytes.
    None (the default value) means split on ASCII whitespace characters
    (space, tab, return, newline, formfeed, vertical tab).
  maxsplit
    Maximum number of splits to do.
    -1 (the default value) means no limit.

Splitting is done starting at the end of the bytes and working to the front.     rstrip($self, bytes=None, /)
--

Strip trailing bytes contained in the argument.

If the argument is omitted or None, strip trailing ASCII whitespace.          split($self, /, sep=None, maxsplit=-1)
--

Return a list of the sections in the bytes, using sep as the delimiter.

  sep
    The delimiter according which to split the bytes.
    None (the default value) means split on ASCII whitespace characters
    (space, tab, return, newline, formfeed, vertical tab).
  maxsplit
    Maximum number of splits to do.
    -1 (the default value) means no limit.    splitlines($self, /, keepends=False)
--

Return a list of the lines in the bytes, breaking at line boundaries.

Line breaks are not included in the resulting list unless keepends is given and
true.           strip($self, bytes=None, /)
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