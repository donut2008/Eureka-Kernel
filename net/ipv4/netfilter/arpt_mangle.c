riptor.
  If this functionality is unavailable, using it raises an exception.
If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, chown will modify the symbolic link itself instead of the file the
  link points to.
It is an error to use dir_fd or follow_symlinks when specifying path as
  an open file descriptor.
dir_fd and follow_symlinks may not be implemented on your platform.
  If they are unavailable, using them will raise a NotImplementedError.              fchown($module, /, fd, uid, gid)
--

Change the owner and group id of the file specified by file descriptor.

Equivalent to os.chown(fd, uid, gid).             lchown($module, /, path, uid, gid)
--

Change the owner and group id of path to the numeric uid and gid.

This function will not follow symbolic links.
Equivalent to os.chown(path, uid, gid, follow_symlinks=False).          chroot($module, /, path)
--

Change root directory to path.     ctermid($module, /)
--

Return the name of the controlling terminal for this process.           getcwd($module, /)
--

Return a unicode string representing the current working directory.      getcwdb($module, /)
--

Return a bytes string representing the current working directory.       link($module, /, src, dst, *, src_dir_fd=None, dst_dir_fd=None,
     follow_symlinks=True)
--

Create a hard link to a file.

If either src_dir_fd or dst_dir_fd is not None, it should be a file
  descriptor open to a directory, and the respective path string (src or dst)
  should be relative; the path will then be relative to that directory.
If follow_symlinks is False, and the last element of src is a symbolic
  link, link will create a link to the symbolic link itself instead of the
  file the link points to.
src_dir_fd, dst_dir_fd, and follow_symlinks may not be implemented on your
  platform.  If they are unavailable, using them will raise a
  NotImplementedError.            listdir($module, /, path=None)
--

Return a list containing the names of the files in the directory.

path can be specified as either str, bytes, or a path-like object.  If path is bytes,
  the filenames returned will also be bytes; in all other circumstances
  the filenames returned will be str.
If path is None,