a3_512([data], *, usedforsecurity=True) -> SHA3 object

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
if_indextoname(index) -- return the corresponding interface name

 [*] not available on all platforms!         _accept() -> (integer, address info)

Wait for an incoming connection.  Return a new socket file descriptor
representing the connection, and the address of the client.
For IP sockets, the address info is a pair (hostaddr, port).            bind(address)

Bind the socket to a local address.  For IP sockets, the address is a
pair (host, port); the host must refer to the local host. For raw packet
sockets the address is a tuple (ifname, proto [,pkttype [,hatype [,addr]]])       close()

Close the socket.  It cannot be used after this call.  connect(address)

Connect the socket to a remote address.  For IP sockets, the address
is a pair (host, port).  connect_ex(address) -> errno

This is like connect(address), but returns an error code (the errno value)
instead of raising an exception when an error occurs.  detach()

Close the socket object without closing the underlying file descriptor.
The object cannot be used after this call, but the file descriptor
can be reused for other purposes.  The file descriptor is returned.        fileno() -> integer

Return the integer file descriptor of the socket.          getpeername() -> address info

Return the address of the remote endpoint.  For IP sockets, the address
info is a pair (hostaddr, port).         getsockname() -> address info

Return the address of the local endpoint.  For IP sockets, the address
info is a pair (hostaddr, port).          getsockopt(level, option[, buffersize]) -> value

Get a socket option.  See the Unix manual for level and option.
If a nonzero buffersize argument is given, the return value is a
string of that length; otherwise it is an integer.           listen([backlog])

Enable a server to accept connections.  If backlog is specified, it must be
at least 0 (if it is lower, it is set to 0); it specifies the number of
unaccepted connections that the system will allow before refusing new
connections. If not specified, a default reasonable value is chosen.               recv(buffersize[, flags]) -> data

Receive up to buffersize bytes from the socket.  For the optional flags
argument, see the Unix manual.  When no data is available, block until
at least one byte is available or until the remote end is closed.  When
the remote end is closed and all data is read, return the empty string.               recv_into(buffer, [nbytes[, flags]]) -> nbytes_read

A version of recv() that stores its data into a buffer rather than creating
a new string.  Receive up to buffersize bytes from the socket.  If buffersize
is not specified (or 0), receive up to the size available in the given buffer.

See recv() for documentation about the flags.    recvfrom(buffersize[, flags]) -> (data, address info)

Like recv(buffersize, flags) but also return the sender's address info.  recvfrom_into(buffer[, nbytes[, flags]]) -> (nbytes, address info)

Like recv_into(buffer[, nbytes[, flags]]) but also return the sender's address info.        send(data[, flags]) -> count

Send a data string to the socket.  For the optional flags
argument, see the Unix manual.  Return the number of bytes
sent; this may be less than len(data) if the network is busy.                sendall(data[, flags])

Send a data string to the socket.  For the optional flags
argument, see the Unix manual.  This calls send() repeatedly
until all data is sent.  If an error occurs, it's impossible
to tell how much data has been sent.                sendto(data[, flags], address) -> count

Like send(data, flags) but allows specifying the destination address.
For IP sockets, 