meter entities (including the external DTD subset).

Possible flag values are XML_PARAM_ENTITY_PARSING_NEVER,
XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE and
XML_PARAM_ENTITY_PARSING_ALWAYS. Returns true if setting the flag
was successful.                UseForeignDTD($self, flag=True, /)
--

Allows the application to provide an artificial external subset if one is not specified as part of the document instance.

This readily allows the use of a 'default' document type controlled by the
application, while still getting the advantage of providing document type
information to the parser. 'flag' defaults to True if not provided.      Python wrapper for Expat parser.                ParserCreate($module, /, encoding=None, namespace_separator=None,
             intern=<unrepresentable>)
--

Return a new XML parser object.    ErrorString($module, code, /)
--

Returns string error for given number.        getrusage($module, who, /)
--

 getrlimit($module, resource, /)
--

            prlimit(pid, resource, [limits])                setrlimit($module, resource, limits, /)
--

    getpagesize($module, /)
--

    struct_rusage: Result from getrusage.

This object may be accessed either as a tuple of
    (utime,stime,maxrss,ixrss,idrss,isrss,minflt,majflt,
    nswap,inblock,oublock,msgsnd,msgrcv,nsignals,nvcsw,nivcsw)
or via the attributes ru_utime, ru_stime, ru_maxrss, and so on. This module supports asynchronous I/O on multiple file descriptors.

*** IMPORTANT NOTICE ***
On Windows, only sockets are supported; on Unix, all file descriptors.            select($module, rlist, wlist, xlist, timeout=None, /)
--

Wait until one or more file descriptors are ready for some kind of I/O.

The first three arguments are iterables of file descriptors to be waited for:
rlist -- wait until ready for reading
wlist -- wait until ready for writing
xlist -- wait for an "exceptional condition"
If only one kind of condition is required, pass [] for the other lists.

A file descriptor is either a socket or file object, or a small integer
gotten from a fileno() method call on one of those.

The optional 4th argument sp