tinuing for length bytes.  posix_fadvise($module, fd, offset, length, advice, /)
--

Announce an intention to access data in a specific pattern.

Announce an intention to access data in a specific pattern, thus allowing
the kernel to make optimizations.
The advice applies to the region of the file specified by fd starting at
offset and continuing for length bytes.
advice is one of POSIX_FADV_NORMAL, POSIX_FADV_SEQUENTIAL,
POSIX_FADV_RANDOM, POSIX_FADV_NOREUSE, POSIX_FADV_WILLNEED, or
POSIX_FADV_DONTNEED.              putenv($module, name, value, /)
--

Change or add an environment variable.      unsetenv($module, name, /)
--

Delete an environment variable.  strerror($module, code, /)
--

Translate an error code to a message string.     fchdir($module, /, fd)
--

Change to the directory of the given file descriptor.

fd must be opened on a directory, not a file.
Equivalent to os.chdir(fd).     fsync($module, /, fd)
--

Force write of fd to disk.            sync($module, /)
--

Force write of everything to disk.         fdatasync($module, /, fd)
--

Force write of fd to disk without forcing update of metadata.     WCOREDUMP($module, status, /)
--

Return True if the process returning status was dumped to a core file.        WIFCONTINUED($module, /, status)
--

Return True if a particular process was continued from a job control stop.

Return True if the process returning status was continued from a
job control stop.             WIFSTOPPED($module, /, status)
--

Return True if the process returning status was stopped.     WIFSIGNALED($module, /, status)
--

Return True if the process returning status was terminated by a signal.     WIFEXITED($module, /, status)
--

Return True if the process returning status exited via the exit() system call.                WEXITSTATUS($module, /, status)
--

Return the process return code from status. WTERMSIG($module, /, status)
--

Return the signal that terminated the process that provided the status value.  WSTOPSIG($module, /, status)
--

Return the signal that stopped the process that provided the status value.     fstatvfs($module, fd, /)
--

Perform an fstatvfs system call on the given fd.

Equi