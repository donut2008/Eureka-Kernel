 Windows if the target is to be
  interpreted as a directory.  (On Windows, symlink requires
  Windows 6.0 or greater, and raises a NotImplementedError otherwise.)
  target_is_directory is ignored on non-Windows platforms.

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.    system($module, /, command)
--

Execute the command in a subshell.              umask($module, mask, /)
--

Set the current numeric umask and return the previous umask.        uname($module, /)
--

Return an object identifying the current operating system.

The object behaves like a named tuple with the following fields:
  (sysname, nodename, release, version, machine)             unlink($module, /, path, *, dir_fd=None)
--

Remove a file (same as remove()).

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.      remove($module, /, path, *, dir_fd=None)
--

Remove a file (same as unlink()).

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.      utime($module, /, path, times=None, *, ns=<unrepresentable>,
      dir_fd=None, follow_symlinks=True)
--

Set the access and modified time of path.

path may always be specified as a string.
On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.

If times is not None, it must be a tuple (atime, mtime);
    atime and mtime should be expressed as float seconds since the epoch.
If ns is specified, it must be a tuple (atime_ns, mtime_ns);
    atime_ns and mtime_ns should be expressed as integer nanoseconds
    since the epoch.
If times is None and ns is unspecified, utime uses the current time.
Specifying tuples for both times and ns is an error.

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, utime will modify the symbolic link itself instead of the file the
  link points to.
It is an error to use dir_fd or follow_symlinks when specifying path
  as an open file descriptor.
dir_fd and follow_symlinks may not be available on your platform.
  If they are unavailable, using them will raise a NotImplementedError.              times($module, /)
--

Return a collection containing process timing information.

The object returned behaves like a named tuple with these fields:
  (utime, stime, cutime, cstime, elapsed_time)
All fields are floating point numbers.       _exit($module, /, status)
--

Exit to the system with specified status, without normal exit processing.         execv($module, path, argv, /)
--

Execute an executable path with arguments, replacing current process.

  path
    Path of executable file.
  argv
    Tuple or list of strings.               execve($module, /, path, argv, env)
--

Execute an executable path with arguments, replacing current process.

  path
    Path of executable file.
  argv
    Tuple or list of strings.
  env
    Dictionary of strings mapping to strings.     fork($module, /)
--

Fork a child process.

Return 0 to child process and PID of child to parent process.       register_at_fork($module, /, *, before=<unrepresentable>,
                 after_in_child=<unrepresentable>,
                 after_in_parent=<unrepresentable>)
--

Register callables to be called when forking a new process.

  before
    A callable to be called in the parent before the fork() syscall.
  after_in_child
    A callable to be called in the child after fork().
  after_in_parent
    A callable to be called in the parent after fork().

'before' callbacks are called in reverse order.
'after_in_child' and 'after_in_parent' callbacks are called in order.        sched_get_priority_max($module, /, policy)
--

Get the maximum scheduling priority for policy.  sched_get_priority_min($module, /, policy)
--

Get the minimum scheduling priority for policy.  sched_getparam($module, pid, /)
--

Returns scheduling parameters for the process identified by pid.

If pid is 0, returns parameters for the calling process.
Return value is an instance of sched_param.      sched_getscheduler($module, pid, /)
--

Get the scheduling policy for the process identified by pid.

Passing 0 for pid returns the scheduling policy for the calling process.  sched_rr_get_interval($module, pid, /)
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

Composes a raw device number from the major and minor device numbers.     ftruncate($module, fd, length, /)
--

Truncate a file, specified by file descriptor, to a specific length.      truncate($module, /, path, length)
--

Truncate a file, specified by path, to a specific length.

On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.               posix_fallocate($module, fd, offset, length, /)
--

Ensure a file has allocated at least a particular number of bytes on disk.

Ensure that the file specified by fd encompasses a range of bytes
starting at offset bytes from the beginning and continuing for length bytes.  posix_fadvise($module, fd, offset, length, advice, /)
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

Equivalent to statvfs(fd).       statvfs($module, /, path)
--

Perform a statvfs system call on the given path.

path may always be specified as a string.
On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.       confstr($module, name, /)
--

Return a string-valued system configuration variable.             sysconf($module, name, /)
--

Return an integer-valued system configuration variable.           fpathconf($module, fd, name, /)
--

Return the configuration limit name for the file descriptor fd.

If there is no limit, return -1.           pathconf($module, /, path, name)
--

Return the configuration limit name for the file or directory path.

If there is no limit, return -1.
On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.      abort($module, /)
--

Abort the interpreter immediately.

This function 'dumps core' or otherwise fails in the hardest way possible
on the hosting operating system.  This function never returns.              getloadavg($module, /)
--

Return average recent system load information.

Return the number of processes in the system run queue averaged over
the last 1, 5, and 15 minutes as a tuple of three floats.
Raises OSError if the load average was unobtainable.  urandom($module, size, /)
--

Return a bytes object containing random bytes suitable for cryptographic use.     setresuid($module, ruid, euid, suid, /)
--

Set the current process's real, effective, and saved user ids.      setresgid($module, rgid, egid, sgid, /)
--

Set the current process's real, effective, and saved group ids.     getresuid($module, /)
--

Return a tuple of the current process's real, effective, and saved user ids.          getresgid($module, /)
--

Return a tuple of the current process's real, effective, and saved group ids.         getxattr($module, /, path, attribute, *, follow_symlinks=True)
--

Return the value of extended attribute attribute on path.

path may be either a string, a path-like object, or an open file descriptor.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, getxattr will examine the symbolic link itself instead of the file
  the link points to.         setxattr($module, /, path, attribute, value, flags=0, *,
         follow_symlinks=True)
--

Set extended attribute attribute on path to value.

path may be either a string, a path-like object,  or an open file descriptor.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, setxattr will modify the symbolic link itself instead of the file
  the link points to.       removexattr($module, /, path, attribute, *, follow_symlinks=True)
--

Remove extended attribute attribute on path.

path may be either a string, a path-like object, or an open file descriptor.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, removexattr will modify the symbolic link itself instead of the file
  the link points to. listxattr($module, /, path=None, *, follow_symlinks=True)
--

Return a list of extended attributes on path.

path may be either None, a string, a path-like object, or an open file descriptor.
if path is None, listxattr will examine the current directory.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, listxattr will examine the symbolic link itself instead of the file
  the link points to.    get_terminal_size($module, fd=<unrepresentable>, /)
--

Return the size of the terminal window as (columns, lines).

The optional argument fd (default standard output) specifies
which file descriptor should be queried.

If the file descriptor is not connected to a terminal, an OSError
is thrown.

This function will only be defined if an implementation is
available for this system.

shutil.get_terminal_size is the high-level function which should
normally be used, os.get_terminal_size is the low-level implementation.       cpu_count($module, /)
--

Return the number of CPUs in the system; return None if indeterminable.

This number is not equivalent to the number of CPUs the current process can
use.  The number of usable CPUs can be obtained with
``len(os.sched_getaffinity(0))``            get_inheritable($module, fd, /)
--

Get the close-on-exe flag of the specified file descriptor. set_inheritable($module, fd, inheritable, /)
--

Set the inheritable flag of the specified file descriptor.     get_blocking($module, fd, /)
--

Get the blocking mode of the file descriptor.

Return False if the O_NONBLOCK flag is set, True if the flag is cleared.        set_blocking($module, fd, blocking, /)
--

Set the blocking mode of the specified file descriptor.

Set the O_NONBLOCK flag if blocking is False,
clear the O_NONBLOCK flag otherwise.          scandir($module, /, path=None)
--

Return an iterator of DirEntry objects for given path.

path can be specified as either str, bytes, or a path-like object.  If path
is bytes, the names of yielded DirEntry objects will also be bytes; in
all other circumstances they will be str.

If path is None, uses the path='.'.    fspath($module, /, path)
--

Return the file system path representation of the object.

If the object is str or bytes, then allow it to pass through as-is. If the
object defines __fspath__(), then return the result of that method. All other
types raise a TypeError.       waitstatus_to_exitcode($module, /, status)
--

Convert a wait status to an exit code.

On Unix:

* If WIFEXITED(status) is true, return WEXITSTATUS(status).
* If WIFSIGNALED(status) is true, return -WTERMSIG(status).
* Otherwise, raise a ValueError.

On Windows, return status shifted right by 8 bits.

On Unix, if the process is being traced or if waitpid() was called with
WUNTRACED option, the caller must first check if WIFSTOPPED(status) is true.
This function must not be called if WIFSTOPPED(status) is true.             waitid_result: Result from waitid.

This object may be accessed either as a tuple of
  (si_pid, si_uid, si_signo, si_status, si_code),
or via the attributes si_pid, si_uid, and so on.

See os.waitid for more information.    stat_result: Result from stat, fstat, or lstat.

This object may be accessed either as a tuple of
  (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)
or via the attributes st_mode, st_ino, st_dev, st_nlink, st_uid, and so on.

Posix/windows: If your platform supports st_blksize, st_blocks, st_rdev,
or st_flags, they are available as attributes only.

See os.stat for more information.   statvfs_result: Result from statvfs or fstatvfs.

This object may be accessed either as a tuple of
  (bsize, frsize, blocks, bfree, bavail, files, ffree, favail, flag, namemax),
or via the attributes f_bsize, f_frsize, f_blocks, f_bfree, and so on.

See os.statvfs for more information.  sched_param(sched_priority)
--

Currently has only one field: sched_priority

  sched_priority
    A scheduling parameter.      A tuple of (columns, lines) for holding terminal window size    is_dir($self, /, *, follow_symlinks=True)
--

Return True if the entry is a directory; cached per entry.        is_file($self, /, *, follow_symlinks=True)
--

Return True if the entry is a file; cached per entry.            is_symlink($self, /)
--

Return True if the entry is a symbolic link; cached per entry.         stat($self, /, *, follow_symlinks=True)
--

Return stat_result object for the entry; cached per entry.          inode($self, /)
--

Return inode of the entry; cached per entry.                __fspath__($self, /)
--

Returns the path for the entry.                        times_result: Result from os.times().

This object may be accessed either as a tuple of
  (user, system, children_user, children_system, elapsed),
or via the attributes user, system, children_user, children_system,
and elapsed.

See os.times for more information.         uname_result: Result from os.uname().

This object may be accessed either as a tuple of
  (sysname, nodename, release, version, machine),
or via the attributes sysname, nodename, release, version, and machine.

See os.uname for more information.           ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/                SERVERINFOV2 FOR                0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz./                http://www.w3.org/XML/1998/namespace            SERVERINFO FOR  �DX���x#U�ōr�l!�6֮I�N��#�|���������������������������?        http://www.w3.org/2000/xmlns/   0123456789ABCDEF                                invalid signal number %ld, please use valid_signals()           !"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr                This module provides mechanisms to use signal handlers in Python.

Functions:

alarm() -- cause SIGALRM after a specified time [Unix only]
setitimer() -- cause a signal (described below) after a specified
               float time and the timer may restart then [Unix only]
getitimer() -- get current value of timer [Unix only]
signal() -- set the action for a given signal
getsignal() -- get the signal action for a given signal
pause() -- wait until a signal arrives [Unix only]
default_int_handler() -- default SIGINT handler

signal constants:
SIG_DFL -- used to refer to the system default handler
SIG_IGN -- used to ignore the signal
NSIG -- number of defined signals
SIGINT, SIGTERM, etc. -- signal numbers

itimer constants:
ITIMER_REAL -- decrements in real time, and delivers SIGALRM upon
               expiration
ITIMER_VIRTUAL -- decrements only when the process is executing,
               and delivers SIGVTALRM upon expiration
ITIMER_PROF -- decrements both when the process is executing and
               when the system is executing on behalf of the process.
               Coupled with ITIMER_VIRTUAL, this timer is usually
               used to profile the time spent by the application
               in user and kernel space. SIGPROF is delivered upon
               expiration.


*** IMPORTANT NOTICE ***
A signal handler function is called with two arguments:
the first is the signal number, the second is the interrupted stack frame.           default_int_handler(...)

The default handler for SIGINT installed by Python.
It raises KeyboardInterrupt.      alarm($module, seconds, /)
--

Arrange for SIGALRM to arrive after the given number of seconds. setitimer($module, which, seconds, interval=0.0, /)
--

Sets given itimer (one of ITIMER_REAL, ITIMER_VIRTUAL or ITIMER_PROF).

The timer will fire after value seconds and after that every interval seconds.
The itimer can be cleared by setting seconds to zero.

Returns old values as a tuple: (delay, interval).         getitimer($module, which, /)
--

Returns current value of given itimer.         signal($module, signalnum, handler, /)
--

Set the action for the given signal.

The action can be SIG_DFL, SIG_IGN, or a callable Python object.
The previous action is returned.  See getsignal() for possible return values.

*** IMPORTANT NOTICE ***
A signal handler function is called with two arguments:
the first is the signal number, the second is the interrupted stack frame.    raise_signal($module, signalnum, /)
--

Send a signal to the executing process. strsignal($module, signalnum, /)
--

Return the system description of the given signal.

The return values can be such as "Interrupt", "Segmentation fault", etc.
Returns None if the signal is not recognized. getsignal($module, signalnum, /)
--

Return the current action for the given signal.

The return value can be:
  SIG_IGN -- if the signal is being ignored
  SIG_DFL -- if the default action for the signal is in effect
  None    -- if an unknown handler is in effect
  anything else -- the callable Python object used as a handler       set_wakeup_fd(fd, *, warn_on_full_buffer=True) -> fd

Sets the fd to be written to (with the signal number) when a signal
comes in.  A library can use this to wakeup select or poll.
The previous fd or -1 is returned.

The fd must be non-blocking.          siginterrupt($module, signalnum, flag, /)
--

Change system call restart behaviour.

If flag is False, system calls will be restarted when interrupted by
signal sig, else system calls will be interrupted.    pause($module, /)
--

Wait until a signal arrives.              pthread_kill($module, thread_id, signalnum, /)
--

Send a signal to a thread.   pthread_sigmask($module, how, mask, /)
--

Fetch and/or change the signal mask of the calling thread.           sigpending($module, /)
--

Examine pending signals.

Returns a set of signal numbers that are pending for delivery to
the calling thread.       sigwait($module, sigset, /)
--

Wait for a signal.

Suspend execution of the calling thread until the delivery of one of the
signals specified in the signal set sigset.  The function accepts the signal
and returns the signal number.        sigwaitinfo($module, sigset, /)
--

Wait synchronously until one of the signals in *sigset* is delivered.

Returns a struct_siginfo containing information about the signal.    sigtimedwait($module, sigset, timeout, /)
--

Like sigwaitinfo(), but with a timeout.

The timeout is specified in seconds, with floating point numbers allowed.                valid_signals($module, /)
--

Return a set of valid signal numbers on this platform.

The signal numbers returned by this function can be safely passed to
functions like `pthread_sigmask`.    struct_siginfo: Result from sigwaitinfo or sigtimedwait.

This object may be accessed either as a tuple of
(si_signo, si_code, si_errno, si_pid, si_uid, si_status, si_band),
or via the attributes si_signo, si_code, and so on.               Debug module to trace memory blocks allocated by Python.        is_tracing($module, /)
--

Return True if the tracemalloc module is tracing Python memory allocations.          clear_traces($module, /)
--

Clear traces of memory blocks allocated by Python. _get_traces($module, /)
--

Get traces of all memory blocks allocated by Python.

Return a list of (size: int, traceback: tuple) tuples.
traceback is a tuple of (filename: str, lineno: int) tuples.

Return an empty list if the tracemalloc module is disabled.              _get_object_traceback($module, obj, /)
--

Get the traceback where the Python object obj was allocated.

Return a tuple of (filename: str, lineno: int) tuples.
Return None if the tracemalloc module is disabled or did not
trace the allocation of the object.                start($module, nframe=1, /)
--

Start tracing Python memory allocations.

Also set the maximum number of frames stored in the traceback of a
trace to nframe.   stop($module, /)
--

Stop tracing Python memory allocations.

Also clear traces of memory blocks allocated by Python.           get_traceback_limit($module, /)
--

Get the maximum number of frames stored in the traceback of a trace.

By default, a trace of an allocated memory block only stores
the most recent frame: the limit is 1.   get_tracemalloc_memory($module, /)
--

Get the memory usage in bytes of the tracemalloc module.

This memory is used internally to trace memory allocations.    get_traced_memory($module, /)
--

Get the current size and peak size of memory blocks traced by tracemalloc.

Returns a tuple: (current: int, peak: int).       reset_peak($module, /)
--

Set the peak size of memory blocks traced by tracemalloc to the current size.

Do nothing if the tracemalloc module is not tracing memory allocations.   /       :                   This module provides access to the garbage collector for reference cycles.

enable() -- Enable automatic garbage collection.
disable() -- Disable automatic garbage collection.
isenabled() -- Returns true if automatic collection is enabled.
collect() -- Do a full collection right now.
get_count() -- Return the current collection counts.
get_stats() -- Return list of dictionaries containing per-generation stats.
set_debug() -- Set debugging flags.
get_debug() -- Get debugging flags.
set_threshold() -- Set the collection thresholds.
get_threshold() -- Return the current the collection thresholds.
get_objects() -- Return a list of all objects tracked by the collector.
is_tracked() -- Returns true if a given object is tracked.
is_finalized() -- Returns true if a given object has been already finalized.
get_referrers() -- Return the list of objects that refer to an object.
get_referents() -- Return the list of objects that an object refers to.
freeze() -- Freeze all tracked objects and ignore them for future collections.
unfreeze() -- Unfreeze all objects in the permanent generation.
get_freeze_count() -- Return the number of objects in the permanent generation.
         enable($module, /)
--

Enable automatic garbage collection.     disable($module, /)
--

Disable automatic garbage collection.   isenabled($module, /)
--

Returns true if automatic garbage collection is enabled.              set_debug($module, flags, /)
--

Set the garbage collection debugging flags.

  flags
    An integer that can have the following bits turned on:
      DEBUG_STATS - Print statistics during collection.
      DEBUG_COLLECTABLE - Print collectable objects found.
      DEBUG_UNCOLLECTABLE - Print unreachable but uncollectable objects
        found.
      DEBUG_SAVEALL - Save objects to gc.garbage rather than freeing them.
      DEBUG_LEAK - Debug leaking programs (everything but STATS).

Debugging information is written to sys.stderr.        get_debug($module, /)
--

Get the garbage collection debugging flags.           get_count($module, /)
--

Return a three-tuple of the current collection counts.                set_threshold(threshold0, [threshold1, threshold2]) -> None

Sets the collection thresholds.  Setting threshold0 to zero disables
collection.
  get_threshold($module, /)
--

Return the current collection thresholds.         collect($module, /, generation=2)
--

Run the garbage collector.

With no arguments, run a full collection.  The optional argument
may be an integer specifying which generation to collect.  A ValueError
is raised if the generation number is invalid.

The number of unreachable objects is returned.       get_objects($module, /, generation=None)
--

Return a list of objects tracked by the collector (excluding the list returned).

  generation
    Generation to extract the objects from.

If generation is not None, return only the objects tracked by the collector
that are in that generation.               get_stats($module, /)
--

Return a list of dictionaries containing per-generation statistics.   is_tracked($module, obj, /)
--

Returns true if the object is tracked by the garbage collector.

Simple atomic objects will return false.       is_finalized($module, obj, /)
--

Returns true if the object has been already finalized by the GC.              get_referrers(*objs) -> list
Return the list of objects that directly refer to any of objs.     get_referents(*objs) -> list
Return the list of objects that are directly referred to by objs.  freeze($module, /)
--

Freeze all current tracked objects and ignore them for future collections.

This can be used before a POSIX fork() call to make the gc copy-on-write friendly.
Note: collection before a POSIX fork() call may free pages for future allocation
which can cause copy-on-write.           unfreeze($module, /)
--

Unfreeze all objects in the permanent generation.

Put all objects in the permanent generation back into oldest generation.            get_freeze_count($module, /)
--

Return the number of objects in the permanent generation.      Module contains faster C implementation of abc.ABCMeta          get_cache_token($module, /)
--

Returns the current ABC cache token.

The token is an opaque object (supporting equality testing) identifying the
current version of the ABC cache for virtual subclasses. The token changes
with every call to register() on any ABC.          _abc_init($module, self, /)
--

Internal ABC helper for class set-up. Should be never used outside abc module.  _reset_registry($module, self, /)
--

Internal ABC helper to reset registry of a given class.

Should be only used by refleak.py                _reset_caches($module, self, /)
--

Internal ABC helper to reset both caches of a given class.

Should be only used by refleak.py               _get_dump($module, self, /)
--

Internal ABC helper for cache and registry debugging.

Return shallow copies of registry, of both caches, and
negative cache version. Don't call this function directly,
instead use ABC._dump_registry() for a nice repr.      _abc_register($module, self, subclass, /)
--

Internal ABC helper for subclasss registration. Should be never used outside abc module.          _abc_instancecheck($module, self, instance, /)
--

Internal ABC helper for instance checks. Should be never used outside abc module.            _abc_subclasscheck($module, self, subclass, /)
--

Internal ABC helper for subclasss checks. Should be never used outside abc module.           Internal state held by ABC machinery.           Accelerator module for asyncio  get_event_loop($module, /)
--

Return an asyncio event loop.

When called from a coroutine or a callback (e.g. scheduled with
call_soon or similar API), this function will always return the
running event loop.

If there is no running event loop set, the function will return
the result of `get_event_loop_policy().get_event_loop()` call.               get_running_loop($module, /)
--

Return the running event loop.  Raise a RuntimeError if there is none.

This function is thread-specific.      _get_running_loop($module, /)
--

Return the running event loop or None.

This is a low-level function intended to be used by event loops.
This function is thread-specific.    _set_running_loop($module, loop, /)
--

Set the running event loop.

This is a low-level function intended to be used by event loops.
This function is thread-specific.         _register_task($module, /, task)
--

Register a new task in asyncio as executed by loop.

Returns None.         _unregister_task($module, /, task)
--

Unregister a task.

Returns None.        _enter_task($module, /, loop, task)
--

Enter into task execution or resume suspended task.

Task belongs to loop.

Returns None.               _leave_task($module, /, loop, task)
--

Leave task execution or suspend a task.

Task belongs to loop.

Returns None.           Future(*, loop=None)
--

This class is *almost* compatible with concurrent.futures.Future.

    Differences:

    - result() and exception() do not take a timeout argument and
      raise an exception when the future isn't done yet.

    - Callbacks registered with add_done_callback() are always called
      via the event loop's call_soon_threadsafe().

    - This class is not compatible with the wait() and as_completed()
      methods in the concurrent.futures package.      result($self, /)
--

Return the result this future represents.

If the future has been cancelled, raises CancelledError.  If the
future's result isn't yet available, raises InvalidStateError.  If
the future is done and has an exception set, this exception is raised.      exception($self, /)
--

Return the exception that was set on this future.

The exception (or None if no exception was set) is returned only if
the future is done.  If the future has been cancelled, raises
CancelledError.  If the future isn't done yet, raises
InvalidStateError.           set_result($self, result, /)
--

Mark the future done and set its result.

If the future is already done when this method is called, raises
InvalidStateError.  set_exception($self, exception, /)
--

Mark the future done and set an exception.

If the future is already done when this method is called, raises
InvalidStateError.          add_done_callback($self, fn, /, *, context=<unrepresentable>)
--

Add a callback to be run when the future becomes done.

The callback is called with a single argument - the future object. If
the future is already done when this is called, the callback is
scheduled with call_soon.       remove_done_callback($self, fn, /)
--

Remove all instances of a callback from the "call when done" list.

Returns the number of callbacks removed.             cancel($self, /, msg=None)
--

Cancel the future and schedule callbacks.

If the future is already done or cancelled, return False.  Otherwise,
change the future's state to cancelled, schedule the callbacks and
return True. cancelled($self, /)
--

Return True if the future was cancelled.                done($self, /)
--

Return True if the future is done.

Done means either that a result / exception are available, or that the
future was cancelled.             get_loop($self, /)
--

Return the event loop the Future is bound to.            _make_cancelled_error($self, /)
--

Create the CancelledError to raise if the Future is cancelled.

This should only be called once when handling a cancellation since
it erases the context exception value.   _repr_info($self, /)
--

       Task(coro, *, loop=None, name=None)
--

A coroutine wrapped in a Future.        set_result($self, result, /)
--

               set_exception($self, exception, /)
--

         cancel($self, /, msg=None)
--

Request that this task cancel itself.

This arranges for a CancelledError to be thrown into the
wrapped coroutine on the next cycle through the event loop.
The coroutine then has a chance to clean up or even deny
the request using try/except/finally.

Unlike Future.cancel, this does not guarantee that the
task will be cancelled: the exception might be caught and
acted upon, delaying cancellation of the task or preventing
cancellation completely.  The task may also return a value or
raise a different exception.

Immediately after this method is called, Task.cancelled() will
not return True (unless the task was already cancelled).  A
task will be marked as cancelled when the wrapped coroutine
terminates with a CancelledError exception (even if cancel()
was not called).        get_stack($self, /, *, limit=None)
--

Return the list of stack frames for this task's coroutine.

If the coroutine is not done, this returns the stack where it is
suspended.  If the coroutine has completed successfully or was
cancelled, this returns an empty list.  If the coroutine was
terminated by an exception, this returns the list of traceback
frames.

The frames are always ordered from oldest to newest.

The optional limit gives the maximum number of frames to
return; by default all available frames are returned.  Its
meaning differs depending on whether a stack or a traceback is
returned: the newest frames of a stack are returned, but the
oldest frames of a traceback are returned.  (This matches the
behavior of the traceback module.)

For reasons beyond our control, only one stack frame is
returned for a suspended coroutine.     print_stack($self, /, *, limit=None, file=None)
--

Print the stack or traceback for this task's coroutine.

This produces output similar to that of the traceback module,
for the frames retrieved by get_stack().  The limit argument
is passed to get_stack().  The file argument is an I/O stream
to which the output is written; by default output is written
to sys.stderr.               _make_cancelled_error($self, /)
--

Create the CancelledError to raise if the Task is cancelled.

This should only be called once when handling a cancellation since
it erases the context exception value.     _repr_info($self, /)
--

       get_name($self, /)
--

         set_name($self, value, /)
--

  get_coro($self, /)
--

         Bisection algorithms.

This module provides support for maintaining a list in sorted order without
having to sort the list after each insertion. For long lists of items with
expensive comparison operations, this can be an improvement over the more
common approach.
       bisect_right($module, /, a, x, lo=0, hi=None)
--

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e <= x, and all e in
a[i:] have e > x.  So if x already appears in the list, i points just
beyond the rightmost x already there

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.      insort_right($module, /, a, x, lo=0, hi=None)
--

Insert item x in list a, and keep it sorted assuming a is sorted.

If x is already in a, insert it to the right of the rightmost x.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.                bisect_left($module, /, a, x, lo=0, hi=None)
--

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e < x, and all e in
a[i:] have e >= x.  So if x already appears in the list, i points 