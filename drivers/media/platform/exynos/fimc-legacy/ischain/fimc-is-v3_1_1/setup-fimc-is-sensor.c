uffer_callback=None)
--

Return the pickled representation of the object as a bytes object.

The optional *protocol* argument tells the pickler to use the given
protocol; supported protocols are 0, 1, 2, 3, 4 and 5.  The default
protocol is 4. It was introduced in Python 3.4, and is incompatible
with previous versions.

Specifying a negative protocol version selects the highest protocol
version supported.  The higher the protocol used, the more recent the
version of Python needed to read the pickle produced.

If *fix_imports* is True and *protocol* is less than 3, pickle will
try to map the new Python 3 names to the old module names used in
Python 2, so that the pickle data stream is readable with Python 2.

If *buffer_callback* is None (the default), buffer views are serialized
into *file* as part of the pickle stream.  It is an error if
*buffer_callback* is not None and *protocol* is None or smaller than 5.    load($module, /, file, *, fix_imports=True, encoding='ASCII',
     errors='strict', buffers=())
--

Read and return an object from the pickle data stored in a file.

This is equivalent to ``Unpickler(file).load()``, but may be more
efficient.

The protocol version of the pickle is detected automatically, so no
protocol argument is needed.  Bytes past the 