uple of
  ((year, week, weekday)
or via the object attributes as named in the above tuple. Abstract base class for time zone info objects.                                                                ;   Z   x   �   �   �   �     0  N              Fixed offset from UTC implementation of tzinfo. date(year, month, day) --> date object          fromtimestamp($type, timestamp, /)
--

Create a date from a POSIX timestamp.

The timestamp is a number, e.g. created via time.time(), that is interpreted
as local time.       datetime(year, month, day[, hour[, minute[, second[, microsecond[,tzinfo]]]]])

The year, month and day arguments are required. tzinfo may be None, or an
instance of a tzinfo subclass. The remaining arguments may be ints.
  now($type, /, tz=None)
--

Returns new datetime object representing current time local to tz.

  tz
    Timezone object.

If no tz is specified, uses local timezone.           time([hour[, minute[, second[, microsecond[, tzinfo]]]]]) --> a time object

All arguments are optional. tzinfo may be None, or an instance of
a tzinfo subclass. The remaining arguments may be ints.
         Difference between two datetime values.

timedelta(days=0, seconds=0, microseconds=0, milliseconds=0, minutes=0, hours=0, weeks=0)

All arguments are optional and default to 0.
Arguments may be integers or floats, and may be positive or negative.          Tools that operate on functions.                reduce(function, sequence[, initial]) -> value

Apply a function of two arguments cumulatively to the items of a sequence,
from left to right, so as to reduce the sequence to a single value.
For example, reduce(lambda x, y: x+y, [1, 2, 3, 4, 5]) calculates
((((1+2)+3)+4)+5).  If initial is present, it is placed before the items
of the sequence in the calculation, and serves as a default when the
sequence is empty.               Convert a cmp= function into a key= function.   partial(func, *args, **keywords) - new function with partial application
    of the given arguments and keywords.
              Create a cached callable that wraps another function.

user_function:      the function being cached

maxsize:  0         for no caching
          None      for unlimited cache size
          n         for a bounded cache

typed:    False     cache f(3) and f(3.0) as identical calls
          True      cache f(3) and f(3.0) as distinct calls

cache_info_type:    namedtuple class with the fields:
                        hits misses currsize maxsize
            �A �A �A �A �A A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A �A A A )A 6A CA PA ]A jA         The object used to calculate HMAC of a message.

Methods:

update() -- updates the current digest with an additional string
digest() -- return the current digest value
hexdigest() -- return the current digest as a string of hexadecimal digits
copy() -- return a copy of the current hash object

Attributes:

name -- the name, including the hash algorithm used by this object
digest_size -- number of bytes in digest() output
       update($self, /, msg)
--

Update the HMAC object with msg.      digest($self, /)
--

Return the digest of the bytes passed to the update() method so far.       hexdigest($self, /)
--

Return hexadecimal digest of the bytes passed to the update() method so far.

This may be used to exchange the value safely in email or other non-binary
environments.  copy($self, /)
--

Return a copy ("clone") of the HMAC object.  new($module, /, name, string=b'', *, usedforsecurity=True)
--

Return a new hash object using the named algorithm.

An optional string argument may be provided and will be
automatically hashed.

The MD5 and SHA1 algorithms are always supported.            pbkdf2_hmac($module, /, hash_name, password, salt, iterations,
            dklen=None)
--

Password based key derivation function 2 (PKCS #5 v2.0) with HMAC as pseudorandom function.          scrypt($module, /, password, *, salt=None, n=None, r=None, p=None,
       maxmem=0, dklen=64)
--

scrypt password-based key derivation function.                get_fips_mode($module, /)
--

Determine the OpenSSL FIPS mode of operation.

For OpenSSL 3.0.0 and newer it returns the state of the default provider
in the default OSSL context. It's not quite the same as FIPS_mode() but good
enough for unittests.

Effectively any non-zero return value indicates FIPS mode;
values other than 1 may have additional significance.      compare_digest($module, a, b, /)
--

Return 'a == b'.

This function uses an approach designed to prevent
timing analysis, making it appropriate for cryptography.

a and b must both be of the same type: either str (ASCII only),
or any bytes-like object.

Note: If a and b are of different lengths, or if an error occurs,
a timing attack could theoretically reveal information about the
types and lengths of a and b--but not their values.           hmac_digest($module, /, key, msg, digest)
--

Single-shot HMAC. hmac_new($module, /, key, msg=b'', digestmod=None)
--

Return a new hmac object.                openssl_md5($module, /, string=b'', *, usedforsecurity=True)
--

Returns a md5 hash object; optionally initialized with a string                openssl_sha1($module, /, string=b'', *, usedforsecurity=True)
--

Returns a sha1 hash object; optionally initialized with a string              openssl_sha224($module, /, string=b'', *, usedforsecurity=True)
--

Returns a sha224 hash object; optionally initialized with a string          openssl_sha256($module, /, string=b'', *, usedforsecurity=True)
--

Returns a sha256 hash object; optionally initialized with a string          openssl_sha384($module, /, string=b'', *, usedforsecurity=True)
--

Returns a sha384 hash object; optionally initialized with a string          openssl_sha512($module, /, string=b'', *, usedforsecurity=True)
--

Returns a sha512 hash object; optionally initialized with a string          openssl_sha3_224($module, /, string=b'', *, usedforsecurity=True)
--

Returns a sha3-224 hash object; optionally initialized with a string      openssl_sha3_256($module, /, string=b'', *, usedforsecurity=True)
--

Returns a sha3-256 hash object; optionally initialized with a string      openssl_sha3_384($module, /, string=b'', *, usedforsecurity=True)
--

Returns a sha3-384 hash object; optionally initialized with a string      openssl_sha3_512($module, /, string=b'', *, usedforsecurity=True)
--

Returns a sha3-512 hash object; optionally initialized with a string      openssl_shake_128($module, /, string=b'', *, usedforsecurity=True)
--

Returns a shake-128 variable hash object; optionally initialized with a string           openssl_shake_256($module, /, string=b'', *, usedforsecurity=True)
--

Returns a shake-256 variable hash object; optionally initialized with a string           HASH(name, string=b'')
--

A hash is an object used to calculate a checksum of a string of information.

Methods:

update() -- updates the current digest with an additional string
digest() -- return the current digest value
hexdigest() -- return the current digest as a string of hexadecimal digits
copy() -- return a copy of the current hash object

Attributes:

name -- the hash algorithm being used by this object
digest_size -- number of bytes in this hashes output           update($self, obj, /)
--

Update this hash object's state with the provided string.             digest($self, /)
--

Return the digest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              copy($self, /)
--

Return a copy of the hash object.            HASHXOF(name, string=b'')
--

A hash is an object used to calculate a checksum of a string of information.

Methods:

update() -- updates the current digest with an additional string
digest(length) -- return the current digest value
hexdigest(length) -- return the current digest as a string of hexadecimal digits
copy() -- return a copy of the current hash object

Attributes:

name -- the hash algorithm being used by this object
digest_size -- number of bytes in this hashes output            digest($self, /, length)
--

Return the digest value as a bytes object.         hexdigest($self, /, length)
--

Return the digest value as a string of hexadecimal digits.      Heap queue algorithm (a.k.a. priority queue).

Heaps are arrays for which a[k] <= a[2*k+1] and a[k] <= a[2*k+2] for
all k, counting elements from 0.  For the sake of comparison,
non-existing elements are considered