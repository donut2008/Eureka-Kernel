able if all of its characters are considered printable in
repr() or if it is empty.        zfill($self, width, /)
--

Pad a numeric string with zeros on the left, to fill a field of the given width.

The string is never truncated.     S.format(*args, **kwargs) -> str

Return a formatted version of S, using substitutions from args and kwargs.
The substitutions are identified by braces ('{' and '}').          S.format_map(mapping) -> str

Return a formatted version of S, using substitutions from mapping.
The substitutions are identified by braces ('{' and '}').      __format__($self, format_spec, /)
--

Return a formatted version of the string as described by format_spec.     maketrans(x, y=<unrepresentable>, z=<unrepresentable>, /)
--

Return a translation table usable for str.translate().

If there is only one argument, it must be a dictionary mapping Unicode
ordinals (integers) or characters to Unicode ordinals, strings or None.
Character keys will be then converted to ordinals.
If there are two arguments, they must be strings of equal length, and
in the resulting dictionary, each character in x will be mapped to the
character at the same position in y. If there is a third argument, it
must be a string, whose characters will be mapped to None in the result.             __sizeof__($self, /)
--

Return the size of the string in memory, in bytes.     Private method returning an estimate of len(list(it)).          Return state information for pickling.          Set state information for unpickling.           Capsule objects let you wrap a C "void *" pointer in a Python
object.  They're a way of passing data through the Python interpreter
without creating your own custom type.

Capsules are used for communication between extension modules.
They provide a way for an extension module to export a C interface
to other extension modules, so that extension modules can use the
Python import mechanism to link to one another.
                (Extremely) low-level import machinery bits as used by importlib and imp.       extension_suffixes($module, /)
--

Returns the list of file suffixes used to identify extension modules.        lock_held($module, /)
--

Return True if the import lock is currently held, else False.

On platforms without threads, return False.            acquire_lock($module, /)
--

Acquires the interpreter's import lock for the current thread.

This lock should be used by import hooks to ensure thread-safety when importing
modules. On platforms without threads, this function does nothing. release_lock($module, /)
--

Release the interpreter's import lock.

On platforms without threads, this function does nothing.  get_frozen_object($module, name, /)
--

Create a code object for a frozen module.               is_frozen_package($module, name, /)
--

Returns True if the module name is of a frozen package. create_builtin($module, spec, /)
--

Create an extension module.                init_frozen($module, name, /)
--

Initializes a frozen module.  is_builtin($module, name, /)
--

Returns True if the module name corresponds to a built-in module.              is_frozen($module, name, /)
--

Returns True if the module name corresponds to a frozen module. create_dynamic($module, spec, file=<unrepresentable>, /)
--

Create an extension module.        exec_dynamic($module, mod, /)
--

Initialize an extension module.               exec_builtin($module, mod, /)
--

Initialize a built-in module. _fix_co_filename($module, code, path, /)
--

Changes code.co_filename to specify the passed-in file path.

  code
    Code object to change.
  path
    File path to use.       source_hash($module, /, key, source)
--

       code(argcount, posonlyargcount, kwonlyargcount, nlocals, stacksize,
      flags, codestring, constants, names, varnames, filename, name,
      firstlineno, lnotab[, freevars[, cellvars]])

Create a code object.  Not for the faint of heart. replace($self, /, *, co_argcount=-1, co_posonlyargcount=-1,
        co_kwonlyargcount=-1, co_nlocals=-1, co_stacksize=-1,
        co_flags=-1, co_firstlineno=-1, co_code=None, co_consts=None,
        co_names=None, co_varnames=None, co_freevars=None,
        co_cellvars=None, co_filename=None, co_name=None,
        co_lnotab=None)
--

Return a copy of the code object with new values for the specified fields.     complex(real=0, imag=0)
--

Create a complex number from a real part and an optional imaginary part.

This is equivalent to (real + imag*1j) where imag defaults to 0.          complex.conjugate() -> complex

Return the complex conjugate of its argument. (3-4j).conjugate() == 3+4j.       complex.__format__() -> str

Convert to a string according to format_spec.      )к= ▄к= Нк= Рк= ╠к= шк=         float(x=0, /)
--

Convert a string or number to a floating point number, if possible.           conjugate($self, /)
--

Return self, the complex conjugate of any float.        __trunc__($self, /)
--

Return the Integral closest to x between 0 and x.       __floor__($self, /)
--

Return the floor as an Integral.        __ceil__($self, /)
--

Return the ceiling as an Integral.       __round__($self, ndigits=None, /)
--

Return the Integral closest to x, rounding half toward even.

When an argument is passed, work like built-in round(x, ndigits).           as_integer_ratio($self, /)
--

Return integer ratio.

Return a pair of integers, whose ratio is exactly equal to the original float
and with a positive denominator.

Raise OverflowError on infinities and a ValueError on NaNs.

>>> (10.0).as_integer_ratio()
(10, 1)
>>> (0.0).as_integer_ratio()
(0, 1)
>>> (-.25).as_integer_ratio()
(-1, 4)              fromhex($type, string, /)
--

Create a floating-point number from a hexadecimal string.

>>> float.fromhex('0x1.ffffp10')
2047.984375
>>> float.fromhex('-0x1p-1074')
-5e-324   hex($self, /)
--

Return a hexadecimal representation of a floating-point number.

>>> (-0.1).hex()
'-0x1.999999999999ap-4'
>>> 3.14159.hex()
'0x1.921f9f01b866ep+1'            is_integer($self, /)
--

Return True if the float is an integer.                __getnewargs__($self, /)
--

   __getformat__($type, typestr, /)
--

You probably don't want to use this function.

  typestr
    Must be 'double' or 'float'.

It exists mainly to be used in Python's test suite.

This function returns whichever of 'unknown', 'IEEE, big-endian' or 'IEEE,
little-endian' best describes the format of floating point numbers used by the
C type named by typestr.         __set_format__($type, typestr, fmt, /)
--

You probably don't want to use this function.

  typestr
    Must be 'double' or 'float'.
  fmt
    Must be one of 'unknown', 'IEEE, big-endian' or 'IEEE, little-endian',
    and in addition can only be one of the latter two if it appears to
    match the underlying C reality.

It exists mainly to be used in Python's test suite.

Override the automatic determination of C-level floating point type.
This affects how floats are converted to and from binary strings.   __format__($self, format_spec, /)
--

Formats the float according to format_spec.               sys.float_info

A named tuple holding information about the float type. It contains low level
information about the precision and internal representation. Please study
your system's :file:`float.h` for more information.                             	                               
                                                                                                                          
                  юЧ= ~■= . = CЧ= CЧ= ЪЧ= ЪЧ=             n_fields        n_sequence_fields               n_unnamed_fields                зH> 9I> GI> _I> wI> ЁI>         tuple(iterable=(), /)
--

Built-in immutable sequence.

If no argument is given, the constructor returns an empty tuple.
If iterable is specified the tuple is initialized from iterable's items.

If the argument is a tuple, the return value is the same object.             __getnewargs__($self, /)
--

   index($self, value, start=0, stop=sys.maxsize, /)
--

Return first index of value.

Raises ValueError if the value is not present.              count($self, value, /)
--

Return number of occurrences of value.               Private method returning an estimate of len(list(it)).          Return state information for pickling.          Set state information for unpickling.           slice(stop)
slice(start, stop[, step])

Create a slice object.  This is used for extended slicing (e.g. a[0:10:2]).             S.indices(len) -> (start, stop, stride)

Assuming a sequence of length len, calculate the start and stop
indices, and the stride length of the extended slice described by
S. Out of bounds indices are clipped in a manner consistent with the
handling of normal slices.      Return state information for pickling.          +Ъ> 7Ъ> CЪ> OЪ> [Ъ> gЪ>         list(iterable=(), /)
--

Built-in mutable sequence.

If no argument is given, the constructor creates a new empty list.
The argument must be an iterable if specified.          __reversed__($self, /)
--

Return a reverse iterator over the list.             __sizeof__($self, /)
--

Return the size of the list in memory, in bytes.       clear($self, /)
--

Remove all items from list. copy($self, /)
--

Return a shallow copy of the list.           append($self, object, /)
--

Append object to the end of the list.              insert($self, index, object, /)
--

Insert object before index. extend($self, iterable, /)
--

Extend list by appending elements from the iterable.             pop($self, index=-1, /)
--

Remove and return item at index (default last).

Raises IndexError if list is empty or index is out of range.       remove($self, value, /)
--

Remove first occurrence of value.

Raises ValueError if the value is not present.   index($self, value, start=0, stop=sys.maxsize, /)
--

Return first index of value.

Raises ValueError if the value is not present.              count($self, value, /)
--

Return number of occurrences of value.               reverse($self, /)
--

Reverse *IN PLACE*.       sort($self, /, *, key=None, reverse=False)
--

Sort the list in ascending order and return None.

The sort is in-place (i.e. the list itself is modified) and stable (i.e. the
order of two equal elements is maintained).

If a key function is given, apply it once to each list item and sort them,
ascending or descending, according to their function values.

The reverse flag can be set to sort in descending order.   Private method returning an estimate of len(list(it)).          Return state information for pickling.          Set state information for unpickling.   ■U? ЯX? ЛX? ВX? ┘X? RW? юV? аV? uW? ѓW? аd? Тd? ╗d? чd? {e? Ёe?         set() -> new empty set object
set(iterable) -> new set object

Build an unordered collection of unique elements.                frozenset() -> empty frozenset object
frozenset(iterable) -> frozenset object

Build an immutable unordered collection of unique elements.      Private method returning an estimate of len(list(it)).          Return state information for pickling.          Add an element to a set.

This has no effect if the element is already present. Remove all elements from this set.              x.__contains__(y) <==> y in x.  Return a shallow copy of a set. Remove an element from a set if it is a member.

If the element is not a member, do nothing.    Return the difference of two or more sets as a new set.

(i.e. all elements that are in this set but not the others.)           Remove all elements of another set from this set.               Return the intersection of two sets as a new set.

(i.e. all elements that are in both sets.)   Update a set with the intersection of itself and another.       Return True if two sets have a null intersection.               Report whether another set contains this set.   Report whether this set contains another set.   Remove and return an arbitrary set element.
Raises KeyError if the set is empty.                Remove an element from a set; it must be a member.

If the element is not a member, raise a KeyError.           S.__sizeof__() -> size of S in memory, in bytes Return the symmetric difference of two sets as a new set.

(i.e. all elements that are in exactly one of the sets.)             Update a set with the symmetric difference of itself and another.               Return the union of sets as a new set.

(i.e. all elements that are in either set.)             Update a set with the union of itself and others.               c                   @   s┬  d Z daddё Zddё Zi Zi ZG ddё deЃZG dd	ё d	ЃZG d
dё dЃZ	G ddё dЃZ
ddё Zddё Zddё ZddюddёZddё Zddё Zddё Zddё ZG d d!ё d!ЃZddd"юd#d$ёZd^d%d&