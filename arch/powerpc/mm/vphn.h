ocal address.  For IP sockets, the address is a
pair (host, port); the host must refer to the local host. For raw packet
sockets the address is a tuple (ifname, proto [,pkttype [,hatype [,addr]]])       close()

Close the socket.  It cannot be used after this call.  connect(address)

Connect the socket to a remote address.  For IP sockets, the address
is a pair (host, port).  connect_ex(address) -> errno

This is like connect(address), but returns an error code (the errno value)
inste