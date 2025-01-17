 address is a pair (hostaddr, port).          setblocking(flag)

Set the socket to blocking (flag is true) or non-blocking (false).
setblocking(True) is equivalent to settimeout(None);
setblocking(False) is equivalent to settimeout(0.0). getblocking()

Returns True if socket is in blocking mode, or False if it
is in non-blocking mode.              settimeout(timeout)

Set a timeout on socket operations.  'timeout' can be a float,
giving in seconds, or None.  Setting a timeout of None disables
the timeout feature and is equivalent to setblocking(1).
Setting a timeout of zero is the same as setblocking(0).           gettimeout() -> timeout

Returns the timeout in seconds (float) associated with socket
operations. A timeout of None indicates that timeouts on socket
operations are disabled. setsockopt(level, option, value: int)
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

Returns the interface name corresponding to the interface index if_index.             CMSG_LEN(length) -> control message length

Return the total length, without trailing padding, of an ancillary
data item with associated data of the given length.  This value can
often be used as the buffer size for recvmsg() to receive a single
item of ancillary data, but RFC 3542 requires portable applications to
use CMSG_SPACE() and thus include space for padding, even when the
item will be the last in the buffer.  Raises OverflowError if length
is outside the permissible range of values.                CMSG_SPACE(length) -> buffer size

Return the buffer size needed for recvmsg() to receive an ancillary
data item with associated data of the given length, along with any
trailing padding.  The buffer space needed to receive multiple items
is the sum of the CMSG_SPACE() values for their associated data
lengths.  Raises OverflowError if length is outside the permissible
range of values.             ԛE ��E ��E ��E ��E ��E ��E ��E ��E ��E k�E ��E J�E r�E r�E r�E r�E r�E ��E r�E r�E ��E j�E \�E I�E d�E ��E )�E v�E ��E ��E ��E ��E ��E ԯE b�E ��E 8�E 8�E ְE �E b�E �E ֱE ��E ��E ��E d�E ��E ��E �E ƲE +�E \�E ��E q�E ��E ��E �E ��E �E �E ��E ܵE ��E ��E C�E ��E ��E ѶE �E +�E u�E շE ��E ��E 1�E ��E P�E ��E ��E $�E 7�E J�E ��E ��E ��E V�E h�E ��E 	�E ��E 1�E �E ��E p�E J�E p�E p�E x�E ��E L�E ��E ��E e�E ��E j�E �E \�E p�E p�E ��E ��E p�E �E y�E P�E ~�E ��E ��E ��E ��E �E u�E C�E ��E ��E ��E h�E ��E ��E ��E �E B�E [�E j�E ��E <�E ��E ��E ��E 3�E ��E M�E ��E ��E ��E 1�E @�E ��E ��E ��E �
F .F C�E ]�E rF �F u�E �F �F ��E �F �F ��E h�E ��E <�E <�E ��E ��E MF �F ��E �F �F �F lF �F &F ��E ��E �E A�E ��E ��E ��E <�E ��E o�E ��E �E ��E ��E ��E ��E H�E ��E ��E ��E ��E f�E ��E :�E #
F -
F q
F #
F �
F �
F $F �	F WF eF �F �F �F �F �F F 0F RF {F �F �F �F �F F !F -F HF cF �F �F �F �F �F �F �F �F �F �F �F �F bF uF 1F �F �F �F �F �F {F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F )F 6F 6F 6F 6F 6F 6F 6F 6F �F �F �F 6F 6F 6F 6F 6F F 6F 6F 6F 6F tF .F 6F 6F 6F 6F 6F 6F 6F 6F 6F 6F 6F 6F 6F 6F 6F 6F FF �F �F F F F F F F F F F F �F F F -F F F F bF F F F F F F F F �F �F F F F bF F F �F �F �F �F �F �F �F �F �F �F �F RF eF !F �F �F �F �F �F kF �F �F �F �F �F uF �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F F &F &F &F &F &F &F &F &F �F �F �F &F &F &F &F &F F &F &F &F &F dF F &F &F &F &F &F &F &F &F &F &F &F &F &F &F &F &F 6F vF F ZF ZF ZF ZF ZF ZF ZF ZF ZF ZF �F ZF ZF �F ZF ZF ZF �F ZF ZF ZF ZF ZF ZF ZF ZF �F F ZF ZF @F �F ZF ZF �F  F �F �F �F �F �F �F �F �F �F BF UF F �F �F �F �F �F [F �F �F �F �F �F eF �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F vF 	F F F F F F F F F �F �F �F F F F F F �F F F F F TF F F F F F F F F F F F F F F F F F &F cF � F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F "F �F �F �F �F �F �F �F �F RF �F �F �F �F , F �F �F x F � F �6F �4F �6F �6F �6F �6F �6F �6F �6F �6F �3F �LF �LF �LF �LF �MF �MF |NF �NF 9RF 9RF 9RF 9RF MF YOF &MF (PF 9RF �LF 4QF 9RF 9RF �LF 9RF 9RF PQF �MF 9RF �MF MF &MF �LF �LF MF &MF �LF �LF MF &MF �LF �LF RF *RF GRF SF SF SF SF SF �RF SF SF SF SF �RF �QF SF SF SF SF SF SF SF SF SF SF SF SF SF SF SF SF �QF          SRE 2.2.2 Copyright (c) 1997-2002 by Secret Labs AB            Compiled regular expression object.             match($self, /, string, pos=0, endpos=sys.maxsize)
--

Matches zero or more characters at the beginning of the string.          fullmatch($self, /, string, pos=0, endpos=sys.maxsize)
--

Matches against all of the string.   search($self, /, string, pos=0, endpos=sys.maxsize)
--

Scan through string looking for a match, and return a corresponding match object instance.

Return None if no position in the string matches.           sub($self, /, repl, string, count=0)
--

Return the string obtained by replacing the leftmost non-overlapping occurrences of pattern in string by the replacement repl.         subn($self, /, repl, string, count=0)
--

Return the tuple (new_string, number_of_subs_made) found by replacing the leftmost non-overlapping occurrences of pattern with the replacement repl.  findall($self, /, string, pos=0, endpos=sys.maxsize)
--

Return a list of all non-overlapping matches of pattern in string.     split($self, /, string, maxsplit=0)
--

Split string by the occurrences of pattern.             finditer($self, /, string, pos=0, endpos=sys.maxsize)
--

Return an iterator over all non-overlapping matches for the RE pattern in string.

For each match, the iterator returns a match object.               scanner($self, /, string, pos=0, endpos=sys.maxsize)
--

       __copy__($self, /)
--

         __deepcopy__($self, memo, /)
--

               The result of re.match() and re.search().
Match objects always have a boolean value of True.    group([group1, ...]) -> str or tuple.
    Return subgroup(s) of the match by indices or names.
    For 0 returns the entire match.              start($self, group=0, /)
--

Return index of the start of the substring matched by group.       end($self, group=0, /)
--

Return index of the end of the substring matched by group.           span($self, group=0, /)
--

For match object m, return the 2-tuple (m.start(group), m.end(group)).              groups($self, /, default=None)
--

Return a tuple containing all the subgroups of the match, from 1.

  default
    Is used for groups that did not participate in the match.   groupdict($self, /, default=None)
--

Return a dictionary containing all the named subgroups of the match, keyed by the subgroup name.

  default
    Is used for groups that did not participate in the match. expand($self, /, template)
--

Return the string obtained by doing backslash substitution on the string template, as done by the sub() method.  __copy__($self, /)
--

         __deepcopy__($self, memo, /)
--

               match($self, /)
--

            search($self, /)
--

           compile($module, /, pattern, flags, code, groups, groupindex,
        indexgroup)
--

          getcodesize($module, /)
--

    