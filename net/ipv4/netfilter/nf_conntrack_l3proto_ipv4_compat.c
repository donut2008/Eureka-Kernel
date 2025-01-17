uce__($self, /)
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
such that sub is contained within S[start:end].  Optiona