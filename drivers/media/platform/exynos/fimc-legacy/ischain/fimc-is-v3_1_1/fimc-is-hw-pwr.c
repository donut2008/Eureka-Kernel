ion; offset may be negative
* 2 -- end of stream; offset is usually negative

Return the new absolute position.               tell($self, /)
--

Return current stream position.              Truncate file to size bytes.

File pointer is left unchanged.  Size defaults to the current IO
position as reported by tell().  Returns the new size.           flush($self, /)
--

Flush write buffers, if applicable.

This is not implemented for read-only and non-blocking streams.        close($self, /)
--

Flush and close the IO object.

This method has no effect if the file is already closed.    seekable($self, /)
--

Return whether object supports random access.

If False, seek(), tell() and truncate() will raise OSError.
This method may need to do a test seek().     readable($self, /)
--

Return whether object was opened for reading.

If False, read() will raise OSError.      writable($self, /)
--

Return whether object was opened for writing.

If False, write() will raise OSError.     fileno($self, /)
--

Returns underlying file descriptor if one exists.

OSError is raised if the IO object does not use a file descriptor.      isatty($self, /)
--

Return whether this is an 'interactive' stream.

Return False if it can't be determined.   readline($self, size=-1, /)
--

Read and return a line from the stream.

If size is specified, at most size bytes will be read.

The line terminator is always b'\n' for binary files; for text
files, the newlines argument to open can be used to select the line
terminator(s) recognized.   readlines($self, hint=-1, /)
--

Return a list of lines from the stream.

hint can be specified to control the number of lines read: no more
lines will be read if the total size (in bytes/characters) of all
lines so far exceeds hint.       writelines($self, lines, /)
--

Write a list of lines to stream.

Line separators are not added, so it is usual for each of the
lines provided to have a line separator at the end.             read($self, size=-1, /)
--

    readall($self, /)
--

Read until EOF, using multiple read() call.               BytesIO(initial_bytes=b'')
--

Buffered I/O implementation using an in-memory bytes buffer.     readable($self, /)
--

Returns True if the IO object can be read.               seek