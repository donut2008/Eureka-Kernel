    The pgroup to use with the POSIX_SPAWN_SETPGROUP flag.
  resetids
    If the value is `true` the POSIX_SPAWN_RESETIDS will be activated.
  setsid
    If the value is `true` the POSIX_SPAWN_SETSID or POSIX_SPAWN_SETSID_NP will be activated.
  setsigmask
    The sigmask to use with the POSIX_SPAWN_SETSIGMASK flag.
  setsigdef
    The sigmask to use with the POSIX_SPAWN_SETSIGDEF flag.
  scheduler
    A tuple with the scheduler policy (optional) and parameters.         posix_spawnp($module, path, argv, env, /, *, file_actions=(),
             setpgroup=<unrepresentable>, resetids=False, setsid=False,
             setsigmask=(), setsigdef=(), scheduler=<unrepresentable>)
--

Execute the program specified by path in a new process.

  path
    Path of executable file.
  argv
    Tuple or list of strings.
  env
    Dictionary of strings mapping to strings.
  file_actions
    A sequence of file action tuples.
  setpgroup
    The pgroup to use with the POSIX_SPAWN_SETPGROUP flag.
  resetids
    If the value is `True` the POSIX_SPAWN_RESETIDS will be activated.
  setsid
    If the value is `True` the POSIX_SPAWN_SETSID or POSIX_SPAWN_SETSID_NP will be activated.
  setsigmask
    The sigmask to use with the POSIX_SPAWN_SETSIGMASK flag.
  setsigdef
    The sigmask to use with the POSIX_SPAWN_SETSIGDEF flag.
  scheduler
    A tuple with the scheduler policy (optional) and parameters.      readlink($module, /, path, *, dir_fd=None)
--

Return a string representing the path to which the symbolic link points.

If dir_fd is not None, it should be a file descriptor open to a directory,
and path should be relative; path will then be relative to that directory.

dir_fd may not be implemented on your platform.  If it is unavailable,
using it will raise a NotImplementedError.               rename($module, /, src, dst, *, src_dir_fd=None, dst_dir_fd=None)
--

Rename a file or directory.

If either src_dir_fd or dst_dir_fd is not None, it should be a file
  descriptor open to a directory, and the respective path string (src or dst)
  should be relative; the path will then be relative to that directory.
src_dir_fd and dst_dir_fd, may not be implemented on your platform.
  I