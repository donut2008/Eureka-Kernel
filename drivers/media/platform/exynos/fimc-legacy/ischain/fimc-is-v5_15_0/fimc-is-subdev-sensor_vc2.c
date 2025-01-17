ine values, as if it had been read from a file using the fromfile() method).  fromunicode($self, ustr, /)
--

Extends this array with data from the unicode string ustr.

The array must be a unicode type array; otherwise a ValueError is raised.
Use array.frombytes(ustr.encode(...)) to append Unicode data to an array of
some other type.              index($self, v, /)
--

Return index of first occurrence of v in the array.      insert($self, i, v, /)
--

Insert a new item v into the array before position i.                pop($self, i=-1, /)
--

Return the i-th element and delete it from the array.

i defaults to -1.                __reduce_ex__($self, value, /)
--

Return state information for pickling.       remove($self, v, /)
--

Remove the first occurrence of v in the array.          reverse($self, /)
--

Reverse the order of the items in the array.              tofile($self, f, /)
--

Write all items (as machine values) to the file object f.               tolist($self, /)
--

Convert array to an ordinary list with the same items.     tobytes($self, /)
--

Convert the array to an array of machine values and return the bytes representation.      tounicode($self, /)
--

Extends this array with data from the unicode string ustr.

Convert the array to a unicode string.  The array must be a unicode type array;
otherwise a ValueError is raised.  Use array.tobytes().decode() to obtain a
unicode string from an array of some other type.                __sizeof__($self, /)
--

Size of the array in memory, in bytes. allow programmer to define multiple exit functions to be executedupon normal program termination.

Two public functions, register and unregister, are defined.
 register(func, *args, **kwargs) -> func

Register a function to be executed upon normal pro