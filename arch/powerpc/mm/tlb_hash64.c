available within that time.
Otherwise ('block' is false), return an item if one is immediately
available, else raise the Empty exception ('timeout' is ignored
in that case).              get_nowait($self, /)
--

Remove and return an item from the queue without blocking.

Only get an item if one is immediately available. Otherwise
raise the Empty exception.     put($self, /, item, block=True, timeout=None)
--

Put the item on the queue.

The optional 'block' and 'timeout' arguments are ignored, as this method
never blocks.  They are provided for compatibility with the Queue class. put_nowait($self, /, item)
--

Put an item into the queue without blocking.

This is exactly equivalent to `put(item)` and is only provided
for compatibility with the Queue class.             qsize($self, /)
--

Return the approximate size of the queue (not reliable!).   Random() -> create a random number generator with its own internal state.       random($self, /)
--

random() -> x in the interval [0, 1).      seed($self, n=None, /)
--

seed([n]) -> None.

Defaults to use urandom and falls back to a combination
of the current time and the process identifier.          getstate($self, /)
--

getstate() -> tuple containing the current state.        setstate($self, state, /)
--

setstate(state) -> None.  Restores generator state.               getrandbits($self, k, /)
--

getrandbits(k) -> x.  Generates an int with k random bits.         Module implements the Mersenne Twister random number generator. copy($self, /)
--

Return a copy of the hash object.            digest($self, /)
--

Return the digest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              update($self, obj, /)
--

Update this hash object's state with the provided string.             sha1($module, /, string=b'', *, usedforsecurity=True)
--

Return a new SHA1 hash object; optionally initialized with a string.  copy($self, /)
--

Return a copy of the hash object.            digest($self, /)
--

Return the digest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              update($self, obj, /)
--

Update this hash object's state with the provided string.             sha256($module, /, string=b'', *, usedforsecurity=True)
--

Return a new SHA-256 hash object; optionally initialized with a string.             sha224($module, /, string=b'', *, usedforsecurity=True)
--

Return a new SHA-224 hash object; optionally initialized with a string.             sha3_224([data], *, usedforsecurity=True) -> SHA3 object

Return a new SHA3 hash object with a hashbit length of 28 bytes.      copy($self, /)
--

Return a copy of the hash object.            digest($self, /)
--

Return the digest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              update($self, data, /)
--

Update this hash object's state with the provided bytes-like object. sha3_256([data], *, usedforsecurity=True) -> SHA3 object

Return a new SHA3 hash object with a hashbit length of 32 bytes.      sha3_384([data], *, usedforsecurity=True) -> SHA3 object

Return a new SHA3 hash object with a hashbit length of 48 bytes.      sha3_512([data], *, usedforsecurity=True) -> SHA3 object

Return a new SHA3 hash object with a hashbit length of 64 bytes.      shake_128([data], *, usedforsecurity=True) -> SHAKE object

Return a new SHAKE hash object.     digest($self, length, /)
--

Return the digest value as a bytes object.         hexdigest($self, length, /)
--

Return the digest value as a string of hexadecimal digits.      shake_256([data], *, usedforsecurity=True) -> SHAKE object

Return a new SHAKE hash object.     copy($self, /)
--

Return a copy of the hash object.            digest($self, /)
--

Return the digest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              update($self, obj, /)
--

Update this hash object's state with the provided string.             sha512($module, /, string=b'', *, usedforsecurity=True)
--

Return a new SHA-512 hash object; optionally initialized with a string.             sha384($module, /, string=b'', *, usedforsecurity=True)
--

Return a new SHA-384 hash object; optionally initialized with a string.             zQE PE �QE �QE �QE �QE �QE �QE �QE �QE �QE �QE �QE �QE �QE VQE hQE �QE �QE �QE PE �QE �QE �QE �QE �QE �QE �QE PE PE �QE �QE �QE �QE �QE �QE �QE �QE IVE �UE VE VE VE VE VE VE VE mVE VE VE VE VE VE %VE 7VE VE VE VE �UE VE VE VE VE VE VE VE �UE �UE VE VE VE VE VE VE VE [VE QWE �VE �WE �WE �WE �WE �WE �WE �WE uWE �WE �WE �WE �WE �WE -WE ?WE �WE �WE �WE �VE �WE �WE �WE �WE �WE �WE �WE �VE �VE �WE �WE �WE �WE �WE �WE �WE cWE D^E L]E �^E �^E �^E �^E �^E �^E �^E Q^E �^E �^E �^E �^E �^E ^^E k^E �^E �^E �^E L]E �^E �^E �^E �^E �^E �^E �^E L]E L]E �^E �^E �^E �^E �^E �^E �^E x^E 'aE  `E waE waE waE waE waE waE waE 7aE waE waE waE waE waE GaE WaE waE waE waE  `E waE waE waE waE waE waE waE  `E  `E waE waE waE waE waE waE waE gaE �zE �{E {E {E {E {E {E {E {E |E {E {E {E {E {E �|E d}E {E {E {E {E {E {E {E {E {E {E {E A{E �|E {E {E {E {E {E {E {E }E G�E ��E ��E ��E ��E ��E ��E ��E ��E ��E ��E ��E ��E ��E ��E ΃E �E ��E ��E ��E ��E ��E ��E ��E ��E ��E ��E ��E 	�E b�E ��E ��E ��E ��E ��E ��E ��E ��E �E ەE ��E ��E ��E ��E ��E ��E ��E ˙E ��E ��E ��E ��E ��E ֙E �E ��E ��E ��E ەE ��E ��E ��E ��E ��E ��E ��E ەE ەE ��E ��E ��E ��E ��E ��E ��E �E socket(family=AF_INET, type=SOCK_STREAM, proto=0) -> socket object
socket(family=-1, type=-1, proto=-1, fileno=None) -> socket object

Open a socket of the given type.  The family argument specifies the
address family; it defaults to AF_INET.  The type argument specifies
whether this is a stream (SOCK_STREAM, this is the default)
or datagram (SOCK_DGRAM) socket.  The protocol argument defaults to 0,
specifying the default protocol.  Keyword arguments are accepted.
The socket is created as non-inheritable.

When a fileno is passed in, family, type and proto are auto-detected,
unless they are explicitly set.

A socket object represents one endpoint of a network connection.

Methods of socket objects (keyword arguments not allowed):

_accept() -- accept connection, returning new socket fd and client address
bind(addr) -- bind the socket to a local address
close() -- close the socket
connect(addr) -- connect the socket to a remote address
connect_ex(addr) -- connect, return an error code instead of an exception
dup() -- return a new socket fd duplicated from fileno()
fileno() -- return underlying file descriptor
getpeername() -- return remote address [*]
getsockname() -- return local address
getsockopt(level, optname[, buflen]) -- get socket options
gettimeout() -- return timeout or None
listen([n]) -- start listening for incoming connections
recv(buflen[, flags]) -- receive data
recv_into(buffer[, nbytes[, flags]]) -- receive data (into a buffer)
recvfrom(buflen[, flags]) -- receive data and sender's address
recvfrom_into(buffer[, nbytes, [, flags])
  -- receive data and sender's address (into a buffer)
sendall(data[, flags]) -- send all data
send(data[, flags]) -- send data, may not send all of it
sendto(data[, flags], addr) -- send data to a given address
setblocking(bool) -- set or clear the blocking I/O flag
getblocking() -- return True if socket is blocking, False if non-blocking
setsockopt(level, optname, value[, optlen]) -- set socket options
settimeout(None | float) -- set or clear the timeout
shutdown(how) -- shut down traffic in one or both directions
if_nameindex() -- return all network interface indices and names
if_nametoindex(name) -- return the corresponding interface index
if_indexton