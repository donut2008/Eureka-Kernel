
setsockopt(level, option, value: buffer)
setsockopt(level, option, None, optlen: int)

Set a socket option.  See the Unix manual for level and option.
The value argument can either be an integer, a string buffer, or
None, optlen.     shutdown(flag)

Shut down the reading side of the socket (flag == SHUT_RD), the writing side
of the socket (flag == SHUT_WR), or both ends (flag == SHUT_RDWR). recvmsg(bufsize[, ancbufsize[, flags]]) -> (data, ancdata, msg_flags, address)

Receive normal data (up to bufsize bytes) and ancillary data from the
socket.  The ancbufsize argument sets the size in bytes of the
internal buffer used to receive the ancillary data; it defaults to 0,
meaning that no ancillary data will be received.  Appropriate buffer
sizes for ancillary data can be calculated using CMSG_SPACE() or
CMSG_LEN(), and items which do not fit into the buffer might be
truncated or discarded.  The flags argument defaults to 0 and has the
same meaning as for recv().

The return value is a 4-tuple: (data, ancdata, msg_flags, address).
The data item is a bytes object holding the non-ancillary data
received.  The ancdata item is a list of zero or more tuples
(cmsg_level, cmsg_type, cmsg_data) representing the ancillary data
(control messages) received: cmsg_level and cmsg_type are integers
specifying the protocol level and protocol-specific type respectively,
and cmsg_data is a bytes object holding the associated data.  The
msg_flags item is the bitwise OR of various flags indicating
conditions on the received message; see your system documentation for
details.  If the receiving socket is unconnected, address is the
address of the sending socket, if available; otherwise, its value is
unspecified.

If recvmsg() raises an exception after the system call returns, it
will first attempt to close any file descriptors received via the
SCM_RIGHTS mechanism.            recvmsg_into(buffers[, ancbufsize[, flags]]) -> (nbytes, ancdata, msg_flags, address)

Receive normal data and ancillary data from the socket, scattering the
non-ancillary data into a series of buffers.  The buffers argument
must be an iterable of objects that export writable buffers
(e.g. bytearray objects); these will be filled with successive chunks
of the non-ancillary data until it has all been written or there are
no more buffers.  The ancbufsize argument sets the size in bytes of
the internal buffer used to receive the ancillary data; it defaults to
0, meaning that no ancillary data will be received.  Appropriate
buffer sizes for ancillary data can be calculated using CMSG_SPACE()
or CMSG_LEN(), and items which do not fit into the buffer might be
truncated or discarded.  The flags argument defaults to 0 and has the
same meaning as for recv().

The return value is a 4-tuple: (nbytes, ancdata, msg_flags, address).
The nbytes item is the total number of bytes of non-ancillary data
written into the buffers.  The ancdata item is a list of zero or more
tuples (cmsg_level, cmsg_type, cmsg_data) representing the ancillary
data (control messages) received: cmsg_level and cmsg_type are
integers specifying the protocol level and protocol-specific type
respectively, and cmsg_data is a bytes object holding the associated
data.  The msg_flags item is the bitwise OR of various flags
indicating conditions on the received message; see your system
documentation for details.  If the receiving socket is unconnected,
address is the address of the sending socket, if available; otherwise,
its value is unspecified.

If recvmsg_into() raises an exception after the system call returns,
it will first attempt to close any file descriptors received via the
SCM_RIGHTS mechanism.      sendmsg(buffers[, ancdata[, flags[, address]]]) -> count

Send normal and ancillary data to the socket, gathering the
non-ancillary data from a series of buffers and concatenating it into
a single message.  The buffers argument specifies the non-ancillary
data as an iterable of bytes-like objects (e.g. bytes objects).
The ancdata argument specifies the ancillary data (control messages)
as an iterable of zero or more tuples (cmsg_level, cmsg_type,
cmsg_data), where cmsg_level and cmsg_type are integers specifying the
protocol level and protocol-specific type respectively, and cmsg_data
is a bytes-like object holding the associated data.  The flags
argument defaults to 0 and has the same meaning as for send().  If
address is supplied and not None, it sets a destination address for
the message.  The return value is the number of bytes of non-ancillary
data sent.         sendmsg_afalg([msg], *, op[, iv[, assoclen[, flags=MSG_MORE]]])

Set operation mode, IV and length of associated data for an AF_ALG
operation socket.           Implementation module for socket operations.

See the socket module for documentation.          gethostbyname(host) -> address

Return the IP address (a string of the form '255.255.255.255') for a host.      gethostbyname_ex(host) -> (name, aliaslist, addresslist)

Return the true host name, a list of aliases, and a list of IP addresses,
for a host.  The host argument is a string giving a host name or IP number. gethostbyaddr(host) -> (name, aliaslist, addresslist)

Return the true host name, a list of aliases, and a list of IP addresses,
for a host.  The host argument is a string giving a host name or IP number.    gethostname() -> string

Return the current host name.          sethostname(name)

Sets the hostname to name.   getservbyname(servicename[, protocolname]) -> integer

Return a port number from a service name and protocol name.
The optional protocol name, if given, should be 'tcp' or 'udp',
otherwise any protocol will match.           getservbyport(port[, protocolname]) -> string

Return the service name from a port number and protocol name.
The optional protocol name, if given, should be 'tcp' or 'udp',
otherwise any protocol will match. getprotobyname(name) -> integer

Return the protocol number for the named protocol.  (Rarely used.)             close(integer) -> None

Close an integer socket file descriptor.  This is like os.close(), but for
sockets; on some platforms os.close() won't work for socket file descriptors.                dup(integer) -> integer

Duplicate an integer socket file descriptor.  This is like os.dup(), but for
sockets; on some platforms os.dup() won't work for socket file descriptors.               socketpair([family[, type [, proto]]]) -> (socket object, socket object)

Create a pair of socket objects from the sockets returned by the platform
socketpair() function.
The arguments are the same as for socket() except the default family is
AF_UNIX if defined on the platform; otherwise, the default is AF_INET.       ntohs(integer) -> integer

Convert a 16-bit unsigned integer from network to host byte order.
Note that in case the received integer does not fit in 16-bit unsigned
integer, but does fit in a positive C int, it is silently truncated to
16-bit unsigned integer.
However, this silent truncation feature is deprecated, and will raise an
exception in future versions of Python.           ntohl(integer) -> integer

Convert a 32-bit integer from network to host byte order.            htons(integer) -> integer

Convert a 16-bit unsigned integer from host to network byte order.
Note that in case the received integer does not fit in 16-bit unsigned
integer, but does fit in a positive C int, it is silently truncated to
16-bit unsigned integer.
However, this silent truncation feature is deprecated, and will raise an
exception in future versions of Python.           htonl(integer) -> integer

Convert a 32-bit integer from host to network byte order.            inet_aton(string) -> bytes giving packed 32-bit IP representation

Convert an IP address in string format (123.45.67.89) to the 32-bit packed
binary format used in low-level network functions.                inet_ntoa(packed_ip) -> ip_address_string

Convert an IP address from 32-bit packed binary format to string format              inet_pton(af, ip) -> packed IP address string

Convert an IP address from string format to a packed string suitable
for use with low-level network functions.   inet_ntop(af, packed_ip) -> string formatted IP address

Convert a packed IP address of the given family to string format.      getaddrinfo(host, port [, family, type, proto, flags])
    -> list of (family, type, proto, canonname, sockaddr)

Resolve host and port into addrinfo struct.   getnameinfo(sockaddr, flags) --> (host, port)

Get host and port for a sockaddr.                getdefaulttimeout() -> timeout

Returns the default timeout in seconds (float) for new socket objects.
A value of None indicates that new socket objects have no timeout.
When the socket module is first imported, the default is None.        setdefaulttimeout(timeout)

Set the default timeout in seconds (float) for new socket objects.
A value of None indicates that new socket objects have no timeout.
When the socket module is first imported, the default is None.                if_nameindex()

Returns a list of network interface information (index, name) tuples.           if_nametoindex(if_name)

Returns the interface index corresponding to the interface name if_name.               if_indextoname(if_index)

Returns the