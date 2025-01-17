hmod($module, /, fd, mode)
--

Change the access permissions of the file given by file descriptor fd.

Equivalent to os.chmod(fd, mode).      chown($module, /, path, uid, gid, *, dir_fd=None, follow_symlinks=True)
--

Change the owner and group id of path to the numeric uid and gid.\

  path
    Path to be examined; can be string, bytes, a path-like object, or open-file-descriptor int.
  dir_fd
    If not None, it should be a file descriptor open to a directory,
    and path should be relative; path will then be relative t