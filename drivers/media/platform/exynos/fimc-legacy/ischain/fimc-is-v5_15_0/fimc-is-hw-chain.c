unt from the
    right, negative values count from the left.

The return value is a bytes object.  This function is also
available as "b2a_hex()".            unhexlify($module, hexstr, /)
--

Binary data of hexadecimal representation.

hexstr must contain an even number of hex digits (upper or lower case).           rlecode_hqx($module, data, /)
--

Binhex RLE-code binary data.  rledecode_hqx($module, data, /)
--

Decode hexbin RLE-coded string.             crc_hqx($module, data, crc, /)
--

Compute CRC-CCITT incrementally.             crc32($module, data, crc=0, /)
--

Compute CRC-32 incrementally.                a2b_qp($module, /, data, header=False)
--

Decode a string of qp-encoded data.  b2a_qp($module, /, data, quotetabs=False, istext=True, header=False)
--

Encode a string using quoted-printable encoding.

On encoding, when istext is set, newlines are not encoded, and white
space at end of lines is.  When istext is not set, \r and \n (CR/LF)
are both encoded.  When quotetabs is set, space and tabs are encoded.      �������������������������������������������>���?456789:;<=��� ��� 	
������ !"#$%&'()*+,-./0123�������������������������������������������������������������������������������������������������������������������������������������}}}}}}}}}}~}}~}}}}}}}}}}}}}}}}}}} 	
}}}}}}}} !"#$}%&'()*+},-./}}}}0123456}789:;<}}=>?}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}  !B c0�@�P�`�p�)�J�k���������1s2R"�R�B�r�b9��{�Z��Ӝ�����b$C4 �d�t�D�Tj�K�(�	������ō�S6r&0�v�f�V�F[�z��8�����׼��H�X�h�x@a(#8���َ��H�i�
�+��Z�J�z�jqP
3:*���˿���y�X�;���l�|�L�\",<`A�������*��h�I��~�n�^�N>2.Qp���������:�Y�x�����ʱ��-�N�o�� �0� P%@Fpg`������ڳ=���^���"�25BRwbVr�˥����n�O�,���4�$��ftGd$TDۧ������_�~��<��&�6��WfvvF4VL�m��/�ș鉊���DXeHx'h���8�(}�\�?����؛����uJTZ7jz�
��*�:.��l�Mͪ����ɍ&|ld\EL�<�,���>�]�|ߛ���ُ��n6~UNt^�.�>��    �0w,a�Q	��m��jp5�c飕d�2�����y�����җ+L�	�|�~-����d�� �jHq���A��}�����mQ���ǅӃV�l��kdz�b���e�O\�lcc=���� n;^iL�A`�rqg���<G�K���k�
����5l��B�ɻ�@����l�2u\�E���Y=ѫ�0�&: �Q�Q��aп���!#ĳV���������(�_���$���|o/LhX�a�=-f��A�vq�� Ҙ*��q���俟3Ը��x4� ��	���j-=m�ld�\c��Qkkbal�0e�N b��l{����W���ٰeP�긾�|�����bI-��|ӌeL��Xa�M�Q�:t ���0��A��Jו�=m�Ѥ����j�iC��n4F�g�и`�s-D�3_L
��|�<qP�A'�� �%�hW��o 	�f���a���^���)"�а����=�Y��.;\���l�� ���������ұt9G��wҝ&���sc�;d�>jm�Zjz���	�'� 
��}D��ң�h���i]Wb��ge�q6l�knv���+ӉZz��J�go߹��ﾎC��Վ�`���~�ѡ���8R��O�g��gW����?K6�H�+�L
��J6`zA��`�U�g��n1y�iF��a��f���o%6�hR�w�G��"/&U�;��(���Z�+j�\����1�е���,��[��d�&�c윣ju
�m�	�?6�grW �J��z��+�{8���Ғ�����|!����ӆB������hn�����[&���w�owG��Z�pj��;f\��e�i�b���kaE�lx�
����T�N³9a&g��`�MGiI�wn>JjѮ�Z��f�@�;�7S���Ş��ϲG���0򽽊º�0��S���$6к���)W�T�g�#.zf��Ja�h]�+o*7������Z��-This module provides access to mathematical functions for complex
numbers.      acos($module, z, /)
--

Return the arc cosine of z.             acosh($module, z, /)
--

Return the inverse hyperbolic cosine of z.             asin($module, z, /)
--

Return the arc sine of z.               asinh($module, z, /)
--

Return the inverse hyperbolic sine of z.               atan($module, z, /)
--

Return the arc tangent of z.            atanh($module, z, /)
--

Return the inverse hyperbolic tangent of z.            cos($module, z, /)
--

Return the cosine of z.  cosh($module, z, /)
--

Return the hyperbolic cosine of z.      exp($module, z, /)
--

Return the exponential value e**z.       isclose($module, /, a, b, *, rel_tol=1e-09, abs_tol=0.0)
--

Determine whether two complex numbers are close in value.

  rel_tol
    maximum difference for being considered "close", relative to the
    magnitude of the input values
  abs_tol
    maximum difference for being considered "close", regardless of the
    magnitude of the input values

Return True if a is close in value to b, and False otherwise.

For the values to be considered close, the difference between them must be
smaller than at least one of the tolerances.

-inf, inf and NaN behave similarly to the IEEE 754 Standard. That is, NaN is
not close to anything, even itself. inf and -inf are only close to themselves.                isfinite($module, z, /)
--

Return True if both the real and imaginary parts of z are finite, else False.       isinf($module, z, /)
--

Checks if the real or imaginary part of z is infinite. isnan($module, z, /)
--

Checks if the real or imaginary part of z not a number (NaN).          log($module, z, base=<unrepresentable>, /)
--

log(z[, base]) -> the logarithm of z to the given base.

If the base not specified, returns the natural logarithm (base e) of z. log10($module, z, /)
--

Return the base-10 logarithm of z.     phase($module, z, /)
--

Return argument, also known as the phase angle, of a complex.          polar($module, z, /)
--

Convert a complex from rectangular coordinates to polar coordinates.

r is the distance from 0 and phi the phase angle.                rect($module, r, phi, /)
--

Convert from polar coordinates to rectangular coordinates.         sin($module, z, /)
--

Return the sine of z.    sinh($module, z, /)
--

Return the hyperbolic sine of z.        sqrt($module, z, /)
--

Return the square root of z.            tan($module, z, /)
--

Return the tangent of z. tanh($module, z, /)
--

Return the hyperbolic tangent of z.     This module makes available standard errno system symbols.

The value of each symbol is the corresponding integer value,
e.g., on most systems, errno.ENOENT equals the integer 2.

The dictionary errno.errorcode maps numeric codes to symbol names,
e.g., errno.errorcode[2] could be the string 'ENOENT'.

Symbols that are not relevant to the underlying system are not defined.

To map error codes to error messages, use the function os.strerror(),
e.g. os.strerror(2) could return 'No such file or directory'.     This module performs file control and I/O control on file
descriptors.  It is an interface to the fcntl() and ioctl() Unix
routines.  File descriptors can be obtained with the fileno() method of
a file or socket object.     fcntl($module, fd, cmd, arg=0, /)
--

Perform the operation `cmd` on file descriptor fd.

The values used for `cmd` are operating system dependent, and are available
as constants in the fcntl module, using the same names as used in
the relevant C header files.  The argument arg is optional, and
defaults to 0; it may be an int or a string.  If arg is given as a string,
the return value of fcntl is a string of that length, containing the
resulting value put in the arg buffer by the operating system.  The length
of the arg string is not allowed to exceed 1024 bytes.  If the arg given
is an integer or if none is specified, the result value is an integer
corresponding to the return value of the fcntl call in the C code.            ioctl($module, fd, request, arg=0, mutate_flag=True, /)
--

Perform the operation `request` on file descriptor `fd`.

The values used for `request` are operating system dependent, and are available
as constants in the fcntl or termios library modules, using the same names as
used in the relevant C header files.

The argument `arg` is optional, and defaults to 0; it may be an int or a
buffer containing character data (most likely a string or an array).

If the argument is a mutable buffer (such as an array) and if the
mutate_flag argument (which is only allowed in this case) is true then the
buffer is (in effect) passed to the operating system and changes made by
the OS will be reflected in the contents of the buffer after the call has
returned.  The return value is the integer returned by the ioctl system
call.

If the argument is a mutable buffer and the mutable_flag argument is false,
the behavior is as if a string had been passed.

If the argument is an immutable buffer (most likely a string) then a copy
of the buffer is passed to the operating system and the return value is a
string of the same length containing whatever the operating system put in
the buffer.  The length of the arg buffer in this case is not allowed to
exceed 1024 bytes.

If the arg given is an integer or if none is specified, the result value is
an integer corresponding to the return value of the ioctl call in the C
code.       flock($module, fd, operation, /)
--

Perform the lock operation `operation` on file descriptor `fd`.

See the Unix manual page for flock(2) for details (On some systems, this
function is emulated using fcntl()).             lockf($module, fd, cmd, len=0, start=0, whence=0, /)
--

A wrapper around the fcntl() locking calls.

`fd` is the file descriptor of the file to lock or unlock, and operation is one
of the following values:

    LOCK_UN - unlock
    LOCK_SH - acquire a shared lock
    LOCK_EX - acquire an exclusive lock

When operation is LOCK_SH or LOCK_EX, it can also be bitwise ORed with
LOCK_NB to avoid blocking on lock acquisition.  If LOCK_NB is used and the
lock cannot be acquired, an OSError will be raised and the exception will
have an errno attribute set to EACCES or EAGAIN (depending on the operating
system -- for portability, check for either value).

`len` is the number of bytes to lock, with the default meaning to lock to
EOF.  `start` is the byte offset, relative to `whence`, to that the lock
starts.  `whence` is as with fileobj.seek(), specifically:

    0 - relative to the start of the file (SEEK_SET)
    1 - relative to the current buffer position (SEEK_CUR)
    2 - relative to the end of the file (SEEK_END)                Access to the Unix group database.

Group entries are reported as 4-tuples containing the following fields
from the group database, in order:

  gr_name   - name of the group
  gr_passwd - group password (encrypted); often empty
  gr_gid    - numeric ID of the group
  gr_mem    - list of members

The gid is an integer, name and password are strings.  (Note that most
users are not explicitly listed as members of the groups they are in
according to the password database.  Check both databases to get
complete membership information.)        getgrgid($module, /, id)
--

Return the group database entry for the given numeric group ID.

If id is not valid, raise KeyError.               getgrnam($module, /, name)
--

Return the group database entry for the given group name.

If name is not valid, raise KeyError. getgrall($module, /)
--

Return a list of all available group entries, in arbitrary order.

An entry whose name starts with '+' or '-' represents an instruction
to use YP/NIS and may not be accessible via getgrnam or getgrgid.              grp.struct_group: Results from getgr*() routines.

This object may be accessed either as a tuple of
  (gr_name,gr_passwd,gr_gid,gr_mem)
or via the object attributes as named in the above tuple.
              Functional tools for creating and using iterators.

Infinite iterators:
count(start=0, step=1) --> start, start+step, start+2*step, ...
cycle(p) --> p0, p1, ... plast, p0, p1, ...
repeat(elem [,n]) --> elem, elem, elem, ... endlessly or up to n times

Iterators terminating on the shortest input sequence:
accumulate(p[, func]) --> p0, p0+p1, p0+p1+p2
chain(p, q, ...) --> p0, p1, ... plast, q0, q1, ...
chain.from_iterable([p, q, ...]) --> p0, p1, ... plast, q0, q1, ...
compress(data, selectors) --> (d[0] if s[0]), (d[1] if s[1]), ...
dropwhile(pred, seq) --> seq[n], seq[n+1], starting when pred fails
groupby(iterable[, keyfunc]) --> sub-iterators grouped by value of keyfunc(v)
filterfalse(pred, seq) --> elements of seq where pred(elem) is False
islice(seq, [start,] stop [, step]) --> elements from
       seq[start:stop:step]
starmap(fun, seq) --> fun(*seq[0]), fun(*seq[1]), ...
tee(it, n=2) --> (it1, it2 , ... itn) splits one iterator into n
takewhile(pred, seq) --> seq[0], seq[1], until pred fails
zip_longest(p, q, ...) --> (p[0], q[0]), (p[1], q[1]), ...

Combinatoric generators:
product(p, q, ... [repeat=1]) --> cartesian product
permutations(p[, r])
combinations(p, r)
combinations_with_replacement(p, r)
       tee($module, iterable, n=2, /)
--

Returns a tuple of n independent iterators.  _tee(iterable, /)
--

Iterator wrapped to make it copyable.     Returns an independent iterator.                Return state information for pickling.          Set state information for unpickling.           teedataobject(iterable, values, next, /)
--

Data container common to multiple tee objects.     accumulate(iterable, func=None, *, initial=None)
--

Return series of accumulated sums (or other binary function results).      combinations(iterable, r)
--

Return successive r-length combinations of elements in the iterable.

combinations(range(4), 3) --> (0,1,2), (0,1,3), (0,2,3), (1,2,3)            Returns size in memory, in bytes.               combinations_with_replacement(iterable, r)
--

Return successive r-length combinations of elements in the iterable allowing individual elements to have successive repeats.

combinations_with_replacement('ABC', 2) --> AA AB AC BB BC CC"     cycle(iterable, /)
--

Return elements from the iterable until it is exhausted. Then repeat the sequence indefinitely.          dropwhile(predicate, iterable, /)
--

Drop items from the iterable while predicate(item) is true.

Afterwards, return every element until the iterable is exhausted.            takewhile(predicate, iterable, /)
--

Return successive entries from an iterable as long as the predicate evaluates to true for each entry.     islice(iterable, stop) --> islice object
islice(iterable, start, stop[, step]) --> islice object

Return an iterator whose next() method returns selected values from an
iterable.  If start is specified, will skip all preceding elements;
otherwise, start defaults to zero.  Step defaults to one.  If
specified as another value, step determines how many values are
skipped between successive calls.  Works like a slice() on a list
but returns an iterator.           starmap(function, iterable, /)
--

Return an iterator whose values are returned from the function evaluated with an argument tuple taken from the given sequence.               chain(*iterables) --> chain object

Return a chain object whose .__next__() method returns elements from the
first iterable until it is exhausted, then elements from the next
iterable, until all of the iterables are exhausted.              from_iterable($type, iterable, /)
--

Alternative chain() constructor taking a single iterable argument that evaluates lazily.  compress(data, selectors)
--

Return data elements corresponding to true selector elements.

Forms a shorter iterator from selected data elements using the selectors to
choose the data elements.              filterfalse(function, iterable, /)
--

Return those items of iterable for which function(item) is false.

If function is None, return the items that are false. count(start=0, step=1)
--

Return a count object whose .__next__() method returns consecutive values.

Equivalent to:
    def count(firstval=0, step=1):
        x = firstval
        while 1:
            yield x
            x += step        zip_longest(iter1 [,iter2 [...]], [fillvalue=None]) --> zip_longest object

Return a zip_longest object whose .__next__() method returns a tuple where
the i-th element comes from the i-th iterable argument.  The .__next__()
method continues until the longest iterable in the argument sequence
is exhausted and then it raises StopIteration.  When the shorter iterables
are exhausted, the fillvalue is substituted in their place.  The fillvalue
defaults to None or can be specified by a keyword argument.
         permutations(iterable, r=None)
--

Return successive r-length permutations of elements in the iterable.

permutations(range(3), 2) --> (0,1), (0,2), (1,0), (1,2), (2,0), (2,1) product(*iterables, repeat=1) --> product object

Cartesian product of input iterables.  Equivalent to nested for-loops.

For example, product(A, B) returns the same as:  ((x,y) for x in A for y in B).
The leftmost iterators are in the outermost for-loop, so the output tuples
cycle in a manner similar to an odometer (with the rightmost element changing
on every iteration).

To compute the product of an iterable with itself, specify the number
of repetitions with the optional repeat keyword argument. For example,
product(A, repeat=4) means the same as product(A, A, A, A).

product('ab', range(3)) --> ('a',0) ('a',1) ('a',2) ('b',0) ('b',1) ('b',2)
product((0,1), (0,1), (0,1)) --> (0,0,0) (0,0,1) (0,1,0) (0,1,1) (1,0,0) ...     repeat(object [,times]) -> create an iterator which returns the object
for the specified number of times.  If not specified, returns the object
endlessly.      Private method returning an estimate of len(list(it)).          groupby(iterable, key=None)
--

make an iterator that returns consecutive keys and groups from the iterable

  iterable
    Elements to divide into groups according to the key function.
  key
    A function for computing the group category for each element.
    If the key function is not specified or is None, the element itself
    is used for grouping.             rH QsH msH |sH �sH IqH �rH �rH �rH �rH ��H ��H чH �H �H     This module provides access to the mathematical functions
defined by the C standard.            acos($module, x, /)
--

Return the arc cosine (measured in radians) of x.

The result is between 0 and pi.      acosh($module, x, /)
--

Return the inverse hyperbolic cosine of x.             asin($module, x, /)
--

Return the arc sine (measured in radians) of x.

The result is between -pi/2 and pi/2.  asinh($module, x, /)
--

Return the inverse hyperbolic sine of x.               atan($module, x, /)
--

Return the arc tangent (measured in radians) of x.

The result is between -pi/2 and pi/2.               atan2($module, y, x, /)
--

Return the arc tangent (measured in radians) of y/x.

Unlike atan(y/x), the signs of both x and y are considered.   atanh($module, x, /)
--

Return the inverse hyperbolic tangent of x.            ceil($module, x, /)
--

Return the ceiling of x as an Integral.

This is the smallest integer >= x.             copysign($module, x, y, /)
--

Return a float with the magnitude (absolute value) of x but the sign of y.

On platforms that support signed zeros, copysign(1.0, -0.0)
returns -1.0.
           cos($module, x, /)
--

Return the cosine of x (measured in radians).            cosh($module, x, /)
--

Return the hyperbolic cosine of x.      degrees($module, x, /)
--

Convert angle x from radians to degrees.             dist($module, p, q, /)
--

Return the Euclidean distance between two points p and q.

The points should be specified as sequences (or iterables) of
coordinates.  Both inputs must have the same dimension.

Roughly equivalent to:
    sqrt(sum((px - qx) ** 2.0 for px, qx in zip(p, q)))     erf($module, x, /)
--

Error function at x.     erfc($module, x, /)
--

Complementary error function at x.      exp($module, x, /)
--

Return e raised to the power of x.       expm1($module, x, /)
--

Return exp(x)-1.

This function avoids the loss of precision involved in the direct evaluation of exp(x)-1 for small x.                fabs($module, x, /)
--

Return the absolute value of the float x.               factorial($module, x, /)
--

Find x!.

Raise a ValueError if x is negative or non-integral.     floor($module, x, /)
--

Return the floor of x as an Integral.

This is the largest integer <= x.               fmod($module, x, y, /)
--

Return fmod(x, y), according to platform C.

x % y may differ.       frexp($module, x, /)
--

Return the mantissa and exponent of x, as pair (m, e).

m is a float and e is an int, such that x = m * 2.**e.
If x is 0, m and e are both 0.  Else 0.5 <= abs(m) < 1.0.               fsum($module, seq, /)
--

Return an accurate floating point sum of values in the iterable seq.

Assumes IEEE-754 floating point arithmetic.     gamma($module, x, /)
--

Gamma function at x.   gcd($module, *integers)
--

Greatest Common Divisor.            hypot(*coordinates) -> value

Multidimensional Euclidean distance from the origin to a point.

Roughly equivalent to:
    sqrt(sum(x**2 for x in coordinates))

For a two dimensional point (x, y), gives the hypotenuse
using the Pythagorean theorem:  sqrt(x*x + y*y).

For example, the hypotenuse of a 3/4/5 right triangle is:

    >>> hypot(3.0, 4.0)
    5.0
          isclose($module, /, a, b, *, rel_tol=1e-09, abs_tol=0.0)
--

Determine whether two floating point numbers are close in value.

  rel_tol
    maximum difference for being considered "close", relative to the
    magnitude of the input values
  abs_tol
    maximum difference for being considered "close", regardless of the
    magnitude of the input values

Return True if a is close in value to b, and False otherwise.

For the values to be considered close, the difference between them
must be smaller than at least one of the tolerances.

-inf, inf and NaN behave similarly to the IEEE 754 Standard.  That
is, NaN is not close to anything, even itself.  inf and -inf are
only close to themselves.       isfinite($module, x, /)
--

Return True if x is neither an infinity nor a NaN, and False otherwise.             isinf($module, x, /)
--

Return True if x is a positive or negative infinity, and False otherwise.              isnan($module, x, /)
--

Return True if x is a NaN (not a number), and False otherwise.         isqrt($module, n, /)
--

Return the integer part of the square root of the input.               lcm($module, *integers)
--

Least Common Multiple.              ldexp($module, x, i, /)
--

Return x * (2**i).

This is essentially the inverse of frexp().     lgamma($module, x, /)
--

Natural logarithm of absolute value of Gamma function at x.           log(x, [base=math.e])
Return the logarithm of x to the given base.

If the base not specified, returns the natural logarithm (base e) of x.     log1p($module, x, /)
--

Return the natural logarithm of 1+x (base e).

The result is computed in a way which is accurate for x near zero.      log10($module, x, /)
--

Return the base 10 logarithm of x.     log2($module, x, /)
--

Return the base 2 logarithm of x.       modf($module, x, /)
--

Return the fractional and integer parts of x.

Both results carry the sign of x and are floats.         pow($module, x, y, /)
--

Return x**y (x to the power of y).    radians($module, x, /)
--

Convert angle x from degrees to radians.             remainder($module, x, y, /)
--

Difference between x and the closest integer multiple of y.

Return x - n*y where n*y is the closest integer multiple of y.
In the case where x is exactly halfway between two multiples of
y, the nearest even value of n is used. The result is always exact. sin($module, x, /)
--

Return the sine of x (measured in radians).              sinh($module, x, /)
--

Return the hyperbolic sine of x.        sqrt($module, x, /)
--

Return the square root of x.            tan($module, x, /)
--

Return the tangent of x (measured in radians).           tanh($module, x, /)
--

Return the hyperbolic tangent of x.     trunc($module, x, /)
--

Truncates the Real x to the nearest Integral toward 0.

Uses the __trunc__ magic method.               prod($module, iterable, /, *, start=1)
--

Calculate the product of all the elements in the input iterable.

The default start value for the product is 1.

When the iterable is empty, return the start value.  This function is
intended specifically for use with numeric values and may reject
non-numeric types.           perm($module, n, k=None, /)
--

Number of ways to choose k items from n items without repetition and with order.

Evaluates to n! / (n - k)! when k <= n and evaluates
to zero when k > n.

If k is not specified or is None, then k defaults to n
and the function returns n!.

Raises TypeError if either of the arguments are not integers.
Raises ValueError if either of the arguments are negative.       comb($module, n, k, /)
--

Number of ways to choose k items from n items without repetition and without order.

Evaluates to n! / (k! * (n - k)!) when k <= n and evaluates
to zero when k > n.

Also called the binomial coefficient because it is equivalent
to the coefficient of k-th term in polynomial expansion of the
expression (1 + x)**n.

Raises TypeError if either of the arguments are not integers.
Raises ValueError if either of the arguments are negative.  nextafter($module, x, y, /)
--

Return the next floating-point value after x towards y.         ulp($module, x, /)
--

Return the value of the least significant bit of the float x.                                               x       �      �      ��      ��      _7      a     ��     �(s    (;L    Xww0   �uw   ���~C   s��   ��+�  ��|g�!              �?      �?       @      @      8@      ^@     ��@     ��@     ��@     &A    ��KA    ��A    ���A   ��2�A   (;L4B  �uwsB  �uw�B  ���7�B  s��6C �h0�{C ZA����C Ƶ�;(Dl�YaRwND        Windows: mmap(fileno, length[, tagname[, access[, offset]]])

Maps length bytes from the file specified by the file handle fileno,
and returns a mmap object.  If length is larger than the current size
of the file, the file is extended to contain length bytes.  If length
is 0, the maximum length of the map is the current size of the file,
except that if the file is empty Windows raises an exception (you cannot
create an empty mapping on Windows).

Unix: mmap(fileno, length[, flags[, prot[, access[, offset]]]])

Ma