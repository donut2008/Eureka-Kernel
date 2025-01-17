 the type of objects stored in them is constrained.
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
    'u'         Unicode character  2 (s