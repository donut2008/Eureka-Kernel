 /)
--

Return a copy with all occurrences of substring old replaced by new.

  count
    Maximum number of occurrences to replace.
    -1 (the default value) means replace all occurrences.

If the optional argument count is given, only the first count occurrences are
replaced.        removeprefix($self, prefix, /)
--

Return a bytes object with the given prefix string removed if present.

If the bytes starts with the prefix string, return bytes[len(prefix):].
Otherwise, return a copy of the original bytes.              removesuffix($self, suffix, /)
--

Return a bytes object with the given suffix string removed if present.

If the bytes ends with the suffix string and that suffix is not empty,
return bytes[:-len(prefix)].  Otherwise, return a copy of the original
bytes. rjust($self, width, fillchar=b' ', /)
--

Return a right-justified string of length width.

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

The original string is never truncated.            Private method returning an estimate of len(list(it)).          Return state information for pickling.          Set state information for unpickling.           ��8@�������@��H��pX�h`xP��0 ��(����� � (0 ��0 P� � 0 x� @ � x (H� � � 8X � H � � � � � @������              type(object_or_name, bases, dict)
type(object) -> the object's type
type(name, bases, dict) -> a new type       object()
--

The base class of the class hierarchy.

When called, it accepts no arguments and returns a new featureless
instance that has no instance attributes and cannot be given any.
      super() -> same as super(__class__, <first argument>)
super(type) -> unbound super object
super(type, obj) -> bound super object; requires isinstance(obj, type)
super(type, type2) -> bound super object; requires issubclass(type2, type)
Typical use to call a cooperative superclass method:
class C(B):
    def meth(self, arg):
        super().meth(arg)
This works for class methods too:
class C(B):
    @classmethod
    def cmeth(cls, arg):
        super().cmeth(arg)
             mro($self, /)
--

Return a type's method resolution order.      __subclasses__($self, /)
--

Return a list of immediate subclasses.             __instancecheck__($self, instance, /)
--

Check if an object is an instance.    __subclasscheck__($self, subclass, /)
--

Check if a class is a subclass.       __dir__($self, /)
--

Specialized __dir__ implementation for types.             __sizeof__($self, /)
--

Return memory consumption of the type object.          __reduce_ex__($self, protocol, /)
--

Helper for pickle.        __reduce__($self, /)
--

Helper for pickle.     Abstract classes can override this to customize issubclass().

This is invoked early on by abc.ABCMeta.__subclasscheck__().
It should return True, False or NotImplemented.  If it returns
NotImplemented, the normal algorithm is used.  Otherwise, it
overrides the normal algorithm (and the outcome is cached).
            This method is called when a class is subclassed.

The default implementation does nothing. It may be
overridden to extend subclasses.
         __format__($self, format_spec, /)
--

Default object formatter. __sizeof__($self, /)
--

Size of object in memory, in bytes.    __dir__($self, /)
--

Default dir() implementation. �n8 Rp8 �n8 �o8 Rp8 Rp8 Rp8 �o8             UnraisableHookArgs

Type used to pass arguments to sys.unraisablehook.          Exception.with_traceback(tb) --
    set self.__traceback__ to tb and return self.               module(name, doc=None)
--

Create a module object.

The name must be a string; the optional doc argument can have any type.     _warnings provides basic warning filtering support.
It is a helper module to speed up interpreter start-up.     warn($module, /, message, category=None, stacklevel=1, source=None)
--

Issue a warning, or maybe ignore it or raise an exception.              Low-level interface to warnings functionality.  Private method returning an estimate of len(list(it)).          Return state information for pickling.          Set state information for unpickling.   F: ��9 ��9 ��9 �: ��9 ��9 E�9 R�9 �: �: l : $: (�9 z: ��9 �9 a�9 ��9 �: �: �: �: �: �9 E: �9 ��9 �: �: Υ9 
�9 	�9 P�9 ��9 6�9 �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �9 ޥ9 ��9 6�9 ��9 q�9 �: �: �9 ��9 ��9 �9 �: �: &�9 �9 	�9 �: ��9 O�9 �: ]�9 2�9 6�9 �9 ��9 ɰ9 ��9 Ƽ9 �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: �: {�9 ��9 �9 {�9 �: ��9 ��9 �9 L�9 �: @�9 o�9 E�9 ��9 m�9 *�9 �9 ��9 ��9 #�9 0�9 ��9 
�9 �9 �9 �9 ��9 {�9 8�9 ��9 ��9 �: �: |�9 �: �9 ��9 ^: g�9 �: x�9 0�9 3�9 ��9 ��9 d�9 �9 ��9 �9 ��9 �: �9 _�9 ��9 X�9 ]�9 ��9 n�9 ��9 8�9 g�9 ��9 �9 ��9 ��9 4�9 ��9 )�9 ��9 6�9 �: �:  �9 y�9 �: ��9 8�9 C�9 �: �: �: �:     send(arg) -> send 'arg' into generator,
return next yielded value or raise StopIteration.       throw(typ[,val[,tb]]) -> raise exception in generator,
return next yielded value or raise StopIteration.        close() -> raise GeneratorExit inside generator.                send(arg) -> send 'arg' into coroutine,
return next iterated value or raise StopIteration.      throw(typ[,val[,tb]]) -> raise exception in coroutine,
return next iterated value or raise StopIteration.       close() -> raise GeneratorExit inside coroutine.                asend(v) -> send 'v' in generator.              athrow(typ[,val[,tb]]) -> raise exception in generator.         aclose() -> raise GeneratorExit inside generator.               L�: Z�: c�: ��: ��: : ��: �: U�: P�: +�: P�: P�: P�: @�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: +�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: U�: P�: P�: P�: :�: P�: :�: P�: P�: P�: v�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: P�: �: %�: %�: \�: %�: %�: P�: P�: P�: P�: P�: %�: ͉:     F.clear(): clear most references held by the frame              F.__sizeof__() -> size of F in memory, in bytes o�: v�: ��: ��: ��: ��:         cell([contents])
--

Create a new cell object.

  contents
    the contents of the cell. If not specified, the cell will be empty,
    and 
 further attempts to access its cell_contents attribute will
    raise a ValueError.                ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: ��: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 8�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: Z�: ��: 	�: ��: ��: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: 	�: |�: ��: 	�: 	�: 	�: 	�: ��: 	�: 	�: 	�: 	�: 	�: 	�: <�: 	�: 	�: o�: 	�: ��: 	�: 	�: ��: �$; H%; �$; �"; �"; �r; �s; ls; �s; �y; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d;  ; d; d; d; d; j~; d; d; d; d; d; d; d; d; ]z; ]z; ]z; ]z; ]z; ]z; ]z; ]z; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; d; }; d; d; d; d; d; d; �; d; d; d; d; d; d; �~; d; d; d; d; �~; �; d; d; d; 2; d; d; d; d; d; d; d; �|; d; d; d; ~; d; �; �|; 8~; d; �|; j�; ��; �; p�; �; Q�; ��; 0�; v�; �; �; v�; v�; Q�; G�; ]�; c�; i�; y�; �; �z< �z< �z< �z< �z< �z< �z< �z< �z< �z< �z< �z< �z< �z< �z< �z< �z< $< $< $< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< �< !�< �< r�< �< $< $< $< �< �< �< �< �< �< �< �< �< �< !�< !�< �< �< �< �< �< :�< ��< ��< ��< ��< :�< ��< ��< ��< ��< ��< �< ��< ��< ��< ��< ��< :�< ��< ��< 5�< ��< ��< T�< T�< ��< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< T�< :�< T�< T�< T�< T�< ��< @�< �< *�< 	�< )�< ?�< `�< ��< �< $�< @�< K�<                                                                                                                                                                                                  str(object='') -> str
str(bytes_or_buffer[, encoding[, errors]]) -> str

Create a new string object from the given object. If encoding or
errors is specified, then the object must expose a data buffer
that will be decoded using the given encoding and error handler.
Otherwise, returns the result of object.__str__() (if defined)
or repr(object).
encoding defaults to sys.getdefaultencoding().
errors defaults to 'strict'.                                                                                                                                    encode($self, /, encoding='utf-8', errors='strict')
--

Encode the string using the codec registered for encoding.

  encoding
    The encoding in which to encode the string.
  errors
    The error handling scheme to use for encoding errors.
    The default is 'strict' meaning that encoding errors raise a
    UnicodeEncodeError.  Other possible values are 'ignore', 'replace' and
    'xmlcharrefreplace' as well as any other name registered with
    codecs.register_error that can handle UnicodeEncodeErrors.  replace($self, old, new, count=-1, /)
--

Return a copy with all occurrences of substring old replaced by new.

  count
    Maximum number of occurrences to replace.
    -1 (the default value) means replace all occurrences.

If the optional argument count is given, only the first count occurrences are
replaced.        split($self, /, sep=None, maxsplit=-1)
--

Return a list of the words in the string, using sep as the delimiter string.

  sep
    The delimiter according which to split the string.
    None (the default value) means split according to any whitespace,
    and discard empty strings from the result.
  maxsplit
    Maximum number of splits to do.
    -1 (the default value) means no limit.            rsplit($self, /, sep=None, maxsplit=-1)
--

Return a list of the words in the string, using sep as the delimiter string.

  sep
    The delimiter according which to split the string.
    None (the default value) means split according to any whitespace,
    and discard empty strings from the result.
  maxsplit
    Maximum number of splits to do.
    -1 (the default value) means no limit.

Splits are done starting at the end of the string and working to the front.              join($self, iterable, /)
--

Concatenate any number of strings.

The string whose method is called is inserted in between each given string.
The result is returned as a new string.

Example: '.'.join(['ab', 'pq', 'rs']) -> 'ab.pq.rs'       capitalize($self, /)
--

Return a capitalized version of the string.

More specifically, make the first character have upper case and the rest lower
case.      casefold($self, /)
--

Return a version of the string suitable for caseless comparisons.        title($self, /)
--

Return a version of the string where each word is titlecased.

More specifically, words start with uppercased characters and all remaining
cased characters have lower case.                center($self, width, fillchar=' ', /)
--

Return a centered string of length width.

Padding is done using the specified fill character (default is a space).   S.count(sub[, start[, end]]) -> int

Return the number of non-overlapping occurrences of substring sub in
string S[start:end].  Optional arguments start and end are
interpreted as in slice notation.          expandtabs($self, /, tabsize=8)
--

Return a copy where all tab characters are expanded using spaces.

If tabsize is not given, a tab size of 8 characters is assumed.          S.find(sub[, start[, end]]) -> int

Return the lowest index in S where substring sub is found,
such that sub is contained within S[start:end].  Optional
arguments start and end are interpreted as in slice notation.

Return -1 on failure.   partition($self, sep, /)
--

Partition the string into three parts using the given separator.

This will search for the separator in the string.  If the separator is found,
returns a 3-tuple containing the part before the separator, the separator
itself, and the part after it.

If the separator is not found, returns a 3-tuple containing the original string
and two empty strings.   S.index(sub[, start[, end]]) -> int

Return the lowest index in S where substring sub is found,
such that sub is contained within S[start:end].  Optional
arguments start and end are interpreted as in slice notation.

Raises ValueError when the substring is not found.     ljust($self, width, fillchar=' ', /)
--

Return a left-justified string of length width.

Padding is done using the specified fill character (default is a space).              lower($self, /)
--

Return a copy of the string converted to lowercase.         lstrip($self, chars=None, /)
--

Return a copy of the string with leading whitespace removed.

If chars is given and not None, remove characters in chars instead.              S.rfind(sub[, start[, end]]) -> int

Return the highest index in S where substring sub is found,
such that sub is contained within S[start:end].  Optional
arguments start and end are interpreted as in slice notation.

Return -1 on failure. S.rindex(sub[, start[, end]]) -> int

Return the highest index in S where substring sub is found,
such that sub is contained within S[start:end].  Optional
arguments start and end are interpreted as in slice notation.

Raises ValueError when the substring is not found.   rjust($self, width, fillchar=' ', /)
--

Return a right-justified string of length width.

Padding is done using the specified fill character (default is a space).             rstrip($self, chars=None, /)
--

Return a copy of the string with trailing whitespace removed.

If chars is given and not None, remove characters in chars instead.             rpartition($self, sep, /)
--

Partition the string into three parts using the given separator.

This will search for the separator in the string, starting at the end. If
the separator is found, returns a 3-tuple containing the part before the
separator, the separator itself, and the part after it.

If the separator is not found, returns a 3-tuple containing two empty strings
and the original string.              splitlines($self, /, keepends=False)
--

Return a list of the lines in the string, breaking at line boundaries.

Line breaks are not included in the resulting list unless keepends is given and
true.          strip($self, chars=None, /)
--

Return a copy of the string with leading and trailing whitespace removed.

If chars is given and not None, remove characters in chars instead.  swapcase($self, /)
--

Convert uppercase characters to lowercase and lowercase characters to uppercase.         translate($self, table, /)
--

Replace each character in the string using the given translation table.

  table
    Translation table, which must be a mapping of Unicode ordinals to
    Unicode ordinals, strings, or None.

The table must implement lookup/indexing via __getitem__, for instance a
dictionary or list.  If this operation raises LookupError, the character is
left untouched.  Characters mapped to None are deleted.     upper($self, /)
--

Return a copy of the string converted to uppercase.         S.startswith(prefix[, start[, end]]) -> bool

Return True if S starts with the specified prefix, False otherwise.
With optional start, test S beginning at that position.
With optional end, stop comparing S at that position.
prefix can also be a tuple of strings to try.   S.endswith(suffix[, start[, end]]) -> bool

Return True if S ends with the specified suffix, False otherwise.
With optional start, test S beginning at that position.
With optional end, stop comparing S at that position.
suffix can also be a tuple of strings to try.       removeprefix($self, prefix, /)
--

Return a str with the given prefix string removed if present.

If the string starts with the prefix string, return string[len(prefix):].
Otherwise, return a copy of the original string.    removesuffix($self, suffix, /)
--

Return a str with the given suffix string removed if present.

If the string ends with the suffix string and that suffix is not empty,
return string[:-len(suffix)]. Otherwise, return a copy of the original
string.        isascii($self, /)
--

Return True if all characters in the string are ASCII, False otherwise.

ASCII characters have code points in the range U+0000-U+007F.
Empty string is ASCII too.         islower($self, /)
--

Return True if the string is a lowercase string, False otherwise.

A string is lowercase if all cased characters in the string are lowercase and
there is at least one cased character in the string.     isupper($self, /)
--

Return True if the string is an uppercase string, False otherwise.

A string is uppercase if all cased characters in the string are uppercase and
there is at least one cased character in the string.    istitle($self, /)
--

Return True if the string is a title-cased string, False otherwise.

In a title-cased string, upper- and title-case characters may only
follow uncased characters and lowercase characters only cased ones.               isspace($self, /)
--

Return True if the string is a whitespace string, False otherwise.

A string is whitespace if all characters in the string are whitespace and there
is at least one character in the string.              isdecimal($self, /)
--

Return True if the string is a decimal string, False otherwise.

A string is a decimal string if all characters in the string are decimal and
there is at least one character in the string.            isdigit($self, /)
--

Return True if the string is a digit string, False otherwise.

A string is a digit string if all characters in the string are digits and there
is at least one character in the string.   isnumeric($self, /)
--

Return True if the string is a numeric string, False otherwise.

A string is numeric if all characters in the string are numeric and there is at
least one character in the string.     isalpha($self, /)
--

Return True if the string is an alphabetic string, False otherwise.

A string is alphabetic if all characters in the string are alphabetic and there
is at least one character in the string.             isalnum($self, /)
--

Return True if the string is an alpha-numeric string, False otherwise.

A string is alpha-numeric if all characters in the string are alpha-numeric and
there is at least one character in the string.    isidentifier($self, /)
--

Return True if the string is a valid Python identifier, False otherwise.

Call keyword.iskeyword(s) to test whether string s is a reserved identifier,
such as "def" or "class".     isprintable($self, /)
--

Return True if the string is printable, False otherwise.

A string is printable if all of its characters are considered printable in
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

Create a code object.  Not for the faint of heart. replace($self, /, *, co_argcount=-1, co_posonl