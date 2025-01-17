fer.
If the return value is an empty bytes instance, this means either
EOF or that no data is available. Use the "eof" property to
distinguish between the two.            write($self, b, /)
--

Writes the bytes b into the memory BIO.

Returns the number of bytes written.            write_eof($self, /)
--

Write an EOF marker to the memory BIO.

When all data has been read, the "eof" property will be True.   The number of bytes pending in the memory BIO.  Whether the memory BIO is at EOF.               Does the session contain a ticket? Session id   Ticket life time hint.          Session creation time (seconds since epoch).    Session timeout (delta in seconds).             Implementation module for SSL socket operations.  See the socket module
for documentation.      _test_decode_cert($module, path, /)
--

        RAND_add($module, string, entropy, /)
--

Mix string into the OpenSSL PRNG state.

entropy (a float) is a lower bound on the entropy contained in
string.  See RFC 4086.        RAND_bytes($module, n, /)
--

Generate n cryptographically strong pseudo-random bytes.          RAND_pseudo_bytes($module, n, /)
--

Generate n pseudo-random bytes.

Return a pair (bytes, is_cryptographic).  is_cryptographic is True
if the bytes generated are cryptographically strong.   RAND_status($module, /)
--

Returns 1 if the OpenSSL PRNG has been seeded with enough data and 0 if not.

It is necessary to seed the PRNG with RAND_add() on some platforms before
using the ssl() function.   get_default_verify_paths($module, /)
--

Return search paths and environment vars that are used by SSLContext's set_default_verify_paths() to load default CAs.

The values are 'cert_file_env', 'cert_file', 'cert_dir_env', 'cert_dir'.       txt2obj($module, /, txt, name=False)
--

Lookup NID, short name, long name and OID of an ASN1_OBJECT.

By default objects are looked up by OID. With name=True short and
long name are also matched.            nid2obj($module, nid, /)
--

Lookup NID, short name, long name and OID of an ASN1_OBJECT by NID.                An error occurred in the SSL implementation.    S_IFMT_: file type bits
S_IFDIR: directory
S_IFCHR: character device
S_IFBLK: block device
S_IFREG: regular file
S_IFIFO: fifo (named pipe)
S_IFLNK: symbolic link
S_IFSOCK: socket file
S_IFDOOR: door
S_IFPORT: event port
S_IFWHT: whiteout

S_ISUID: set UID bit
S_ISGID: set GID bit
S_ENFMT: file locking enforcement
S_ISVTX: sticky bit
S_IREAD: Unix V7 synonym for S_IRUSR
S_IWRITE: Unix V7 synonym for S_IWUSR
S_IEXEC: Unix V7 synonym for S_IXUSR
S_IRWXU: mask for owner permissions
S_IRUSR: read by owner
S_IWUSR: write by owner
S_IXUSR: execute by owner
S_IRWXG: mask for group permissions
S_IRGRP: read by group
S_IWGRP: write by group
S_IXGRP: execute by group
S_IRWXO: mask for others (not in group) permissions
S_IROTH: read by others
S_IWOTH: write by others
S_IXOTH: execute by others

UF_NODUMP: do not dump file
UF_IMMUTABLE: file may not be changed
UF_APPEND: file may only be appended to
UF_OPAQUE: directory is opaque when viewed through a union stack
UF_NOUNLINK: file may not be renamed or deleted
UF_COMPRESSED: OS X: file is hfs-compressed
UF_HIDDEN: OS X: file should not be displayed
SF_ARCHIVED: file may be archived
SF_IMMUTABLE: file may not be changed
SF_APPEND: file may only be appended to
SF_NOUNLINK: file may not be renamed or deleted
SF_SNAPSHOT: file is a snapshot file

ST_MODE
ST_INO
ST_DEV
ST_NLINK
ST_UID
ST_GID
ST_SIZE
ST_ATIME
ST_MTIME
ST_CTIME

FILE_ATTRIBUTE_*: Windows file attribute constants
                   (only present on Windows)
         S_ISDIR(mode) -> bool

Return True if mode is from a directory. S_ISCHR(mode) -> bool

Return True if mode is from a character special device file.             S_ISBLK(mode) -> bool

Return True if mode is from a block special device file. S_ISREG(mode) -> bool

Return True if mode is from a regular file.              S_ISFIFO(mode) -> bool

Return True if mode is from a FIFO (named pipe).        S_ISLNK(mode) -> bool

Return True if mode is from a symbolic link.             S_ISSOCK(mode) -> bool

Return True if mode is from a socket.   S_ISDOOR(mode) -> bool

Return True if mode is from a door.     S_ISPORT(mode) -> bool

Return True if mode is from an event port.              S_ISWHT(mode) -> bool

Return True if mode is from a whiteout.  Return the portion of the file's mode that can be set by os.chmod().            Return the portion of the file's mode that describes the file type.             Convert a file's mode to a string of the form '-rwxrwxrwx' pc?d?b?-?l?s         Accelerators for the statistics module.
        _normal_dist_inv_cdf($module, p, mu, sigma, /)
--

             ��F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F 	�F �F �F ��F 	�F �F Functions to convert between Python values and C structs.
Python bytes objects are used to hold the data representing the C struct
and also as format strings (explained below) to describe the layout of data
in the C struct.

The optional first format char indicates byte order, size and alignment:
  @: native order, size & alignment (default)
  =: native order, std. size & alignment
  <: little-endian, std. size & alignment
  >: big-endian, std. size & alignment
  !: same as >

The remaining chars indicate types of args and must match exactly;
these can be preceded by a decimal repeat count:
  x: pad byte (no data); c:char; b:signed byte; B:unsigne