 process.  sched_rr_get_interval($module, pid, /)
--

Return the round-robin quantum for the process identified by pid, in seconds.

Value returned is a float.            sched_setparam($module, pid, param, /)
--

Set scheduling parameters for the process identified by pid.

If pid is 0, sets parameters for the calling process.
param should be an instance of sched_param.      sched_setscheduler($module, pid, policy, param, /)
--

Set the scheduling policy for the process identified by pid.

If pid is 0, the calling process is changed.
param is an instance of sched_param.          sched_yield($module, /)
--

Voluntarily relinquish the CPU.     sched_setaffinity($module, pid, mask, /)
--

Set the CPU affinity of the process identified by pid to mask.

mask should be an iterable of integers identifying CPUs.           sched_getaffinity($module, pid, /)
--

Return the affinity of the process identified by pid (or the current process if zero).

The affinity is returned as a set of CPU identifiers.            openpty($module, /)
--

Open a pseudo-terminal.

Return a tuple of (master_fd, slave_fd) containing open file descriptors
for both the master and slave ends.   forkpty($module, /)
--

Fork a new process with a new pseudo-terminal as controlling tty.

Returns a tuple of (pid, master_fd).
Like fork(), return pid of 0 to the child process,
and pid of child to the parent process.
To both, return fd of newly opened pseudo-terminal.  getegid($module, /)
--

Return the current process's effective group id.        geteuid($module, /)
--

Return the current process's effective user id.         getgid($module, /)
--

Return the current process's group id.   getgrouplist($module, user, group, /)
--

Returns a list of groups to which a user belongs.

  user
    username to lookup
  group
    base group id of the user                getgroups($module, /)
--

Return list of supplemental group IDs for the process.                getpid($module, /)
--

Return the current process id.           getpgrp($module, /)
--

Return the current process group id.    getppid($module, /)
--

Return the parent's process id.

If the parent process has already exited, Windows machines will still
return its id; others systems will return the id of the 'init' process (1).      getuid($module, /)
--

Return the current process's user id.    getlogin($module, /)
--

Return the actual login name.          kill($module, pid, signal, /)
--

Kill a process with a signal. killpg($module, pgid, signal, /)
--

Kill a process group with a signal.        setuid($module, uid, /)
--

Set the current process's user id.  seteuid($module, euid, /)
--

Set the current process's effective user id.      setreuid($module, ruid, euid, /)
--

Set the current process's real and effective user ids.     setgid($module, gid, /)
--

Set the current process's group id. setegid($module, egid, /)
--

Set the current process's effective group id.     setregid($module, rgid, egid, /)
--

Set the current process's real and effective group ids.    setgroups($module, groups, /)
--

Set the groups of the current process to list.                initgroups($module, username, gid, /)
--

Initialize the group access list.

Call the system initgroups() to initialize the group access list with all of
the groups of which the specified username is a member, plus the specified
group id.  getpgid($module, /, pid)
--

Call the system call getpgid(), and return the result.             setpgrp($module, /)
--

Make the current process the leader of its process group.               wait($module, /)
--

Wait for completion of a child process.

Returns a tuple of information about the child process:
    (pid, status)         wait3($module, /, options)
--

Wait for completion of a child process.

Returns a tuple of information about the child process:
  (pid, status, rusage)         wait4($module, /, pid, options)
--

Wait for completion of a specific child process.

Returns a tuple of information about the child process:
  (pid, status, rusage)           waitid($module, idtype, id, options, /)
--

Returns the result of waiting for a process or processes.

  idtype
    Must be one of be P_PID, P_PGID or P_ALL.
  id
    The id to wait on.
  options
    Constructed from the ORing of one or more of WEXITED, WSTOPPED
    or WCONTINUED and additionally may be ORed with WNOHANG or WNOWAIT.

Returns either waitid_result or None if WNOHANG is specified and there are
no children in a waitable state.     waitpid($module, pid, options, /)
--

Wait for completion of a given child process.

Returns a tuple of information regarding the child process:
    (pid, status)

The options argument is ignored on Windows. getsid($module, pid, /)
--

Call the system call getsid(pid) and return the result.             setsid($module, /)
--

Call the system call setsid().           setpgid($module, pid, pgrp, /)
--

Call the system call setpgid(pid, pgrp).     tcgetpgrp($module, fd, /)
--

Return the process group associated with the terminal specified by fd.            tcsetpgrp($module, fd, pgid, /)
--

Set the process group associated with the terminal specified by fd.         open($module, /, path, flags, mode=511, *, dir_fd=None)
--

Open a file for low level IO.  Returns a file descriptor (integer).

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.     close($module, /, fd)
--

Close a file descriptor.              closerange($module, fd_low, fd_high, /)
--

Closes all file descriptors in [fd_low, fd_high), ignoring errors.  device_encoding($module, /, fd)
--

Return a string describing the encoding of a terminal's file descriptor.

The file descriptor must be attached to a terminal.
If the device is not a terminal, return None. dup($module, fd, /)
--

Return a duplicate of a file descriptor.                dup2($module, /, fd, fd2, inheritable=True)
--

Duplicate file descriptor.      lockf($module, fd, command, length, /)
--

Apply, test or remove a POSIX lock on an open file descriptor.

  fd
    An open file descriptor.
  command
    One of F_LOCK, F_TLOCK, F_ULOCK or F_TEST.
  length
    The number of bytes to lock, starting at the current position.               lseek($module, fd, position, how, /)
--

Set the position of a file descriptor.  Return the new position.

Return the new cursor position in number of bytes
relative to the beginning of the file.             read($module, fd, length, /)
--

Read from a file descriptor.  Returns a bytes object.          readv($module, fd, buffers, /)
--

Read from a file descriptor fd into an iterable of buffers.

The buffers should be mutable buffers accepting bytes.
readv will transfer data into each buffer until it is full
and then move on to the next buffer in the sequence to hold
the rest of the data.

readv returns the total number of bytes read,
which may be less than the total capacity of all the buffers.                pread($module, fd, length, offset, /)
--

Read a number of bytes from a file descriptor starting at a particular offset.

Read length bytes from file descriptor fd, starting at offset bytes from
the beginning of the file.  The file offset remains unchanged.               preadv($module, fd, buffers, offset, flags=0, /)
--

Reads from a file descriptor into a number of mutable bytes-like objects.

Combines the functionality of readv() and pread(). As readv(), it will
transfer data into each buffer until it is full and then move on to the next
buffer in the sequence to hold the rest of the data. Its fourth argument,
specifies the file offset at which the input operation is to be performed. It
will return the total number of bytes read (which can be less than the total
capacity of all the objects).

The flags argument contains a bitwise OR of zero or more of the following flags:

- RWF_HIPRI
- RWF_NOWAIT

Using non-zero flags requires Linux 4.6 or newer.           write($module, fd, data, /)
--

Write a bytes object to a file descriptor.      writev($module, fd, buffers, /)
--

Iterate over buffers, and write the contents of each to a file descriptor.

Returns the total number of bytes written.
buffers must be a sequence of bytes-like objects.    pwrite($module, fd, buffer, offset, /)
--

Write bytes to a file descriptor starting at a particular offset.

Write buffer to fd, starting at offset bytes from the beginning of
the file.  Returns the number of bytes writte.  Does not change the
current file offset.       pwritev($module, fd, buffers, offset, flags=0, /)
--

Writes the contents of bytes-like objects to a file descriptor at a given offset.

Combines the functionality of writev() and pwrite(). All buffers must be a sequence
of bytes-like objects. Buffers are processed in array order. Entire contents of first
buffer is written before proceeding to second, and so on. The operating system may
set a limit (sysconf() value SC_IOV_MAX) on the number of buffers that can be used.
This function writes the contents of each object to the file descriptor and returns
the total number of bytes written.

The flags argument contains a bitwise OR of zero or more of the following flags:

- RWF_DSYNC
- RWF_SYNC

Using non-zero flags requires Linux 4.7 or newer.   sendfile($module, /, out_fd, in_fd, offset, count)
--

Copy count bytes from file descriptor in_fd to file descriptor out_fd.   fstat($module, /, fd)
--

Perform a stat system call on the given file descriptor.

Like stat(), but for an open file descriptor.
Equivalent to os.stat(fd).    isatty($module, fd, /)
--

Return True if the fd is connected to a terminal.

Return True if the file descriptor is an open file descriptor
connected to the slave end of a terminal.           pipe($module, /)
--

Create a pipe.

Returns a tuple of two file descriptors:
  (read_fd, write_fd)             pipe2($module, flags, /)
--

Create a pipe with flags set atomically.

Returns a tuple of two file descriptors:
  (read_fd, write_fd)

flags can be constructed by ORing together one or more of these values:
O_NONBLOCK, O_CLOEXEC.           mkfifo($module, /, path, mode=438, *, dir_fd=None)
--

Create a "fifo" (a POSIX named pipe).

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.        mknod($module, /, path, mode=384, device=0, *, dir_fd=None)
--

Create a node in the file system.

Create a node in the file system (file, device special file or named pipe)
at path.  mode specifies both the permissions to use and the
type of node to be created, being combined (bitwise OR) with one of
S_IFREG, S_IFCHR, S_IFBLK, and S_IFIFO.  If S_IFCHR or S_IFBLK is set on mode,
device defines the newly created device special file (probably using
os.makedev()).  Otherwise device is ignored.

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.     major($module, device, /)
--

Extracts a device major number from a raw device number.          minor($module, device, /)
--

Extracts a device minor number from a raw device number.          makedev($module, major, minor, /)
--

Composes a raw device number from the major and minor devi