gest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              update($self, data, /)
--

Update this hash object's state with the provided bytes-like object. blake2s(data=b'', /, *, digest_size=_blake2.blake2s.MAX_DIGEST_SIZE,
        key=b'', salt=b'', person=b'', fanout=1, depth=1, leaf_size=0,
        node_offset=0, node_depth=0, inner_size=0, last_node=False,
        usedforsecurity=True)
--

Return a new BLAKE2s hash object.             copy($self, /)
--

Return a copy of the hash object.            digest($self, /)
--

Return the digest value as a bytes object. hexdigest($self, /)
--

Return the digest value as a string of hexadecimal digits.              update($self, data, /)
--

Update this hash object's state with the provided bytes-like object. �QG RG �QG �QG "RG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG �QG     register($module, search_function, /)
--

Register a codec search function.

Search functions are expected to take one argument, the encoding name in
all lower case letters, and either return None, or a tuple of functions
(encoder, decoder, stream_reader, stream_writer) (or a CodecInfo object).         lookup($module, encoding, /)
--

Looks up a codec tuple in the Python codec registry and returns a CodecInfo object.            encode($module, /, obj, encoding='utf-8', errors='strict')
--

Encodes obj using the codec registered for encoding.

The default encoding is 'utf-8'.  errors may be given to set a
different error handling scheme.  Default is 'strict' meaning that encoding
errors raise a ValueError.  Other possible values are 'ignore', 'replace'
and 'backslashreplace' as well as any other name registered with
codecs.register_error that can handle ValueErrors.   decode($module, /, obj, encoding='utf-8', errors='strict')
--

Decodes obj using the codec registered for encoding.

Default encoding is 'utf-8'.  errors may be given to set a
different error handling scheme.  Default is 'strict' meaning that encoding
errors raise a ValueError.  Other possible values are 'ignore', 'replace'
and 'backslashreplace' as well as any other name registered with
codecs.register_error that can handle ValueErrors.       escape_encode($module, data, errors=None, /)
--

               escape_decode($module, data, errors=None, /)
--

               utf_8_encode($module, str, errors=None, /)
--

 utf_8_decode($module, data, errors=None, final=False, /)
--

   utf_7_encode($module, str, errors=None, /)
--

 utf_7_decode($module, data, errors=None, final=False, /)
--

   utf_16_encode($module, str, errors=None, byteorder=0, /)
--

   utf_16_le_encode($module, str, errors=None, /)
--

             utf_16_be_encode($module, str, errors=None, /)
--

             utf_16_decode($module, data, errors=None, final=False, /)
--

  utf_16_le_decode($module, data, errors=None, final=False, /)
--

               utf_16_be_decode($module, data, errors=None, final=False, /)
--

               utf_16_ex_decode($module, data, errors=None, byteorder=0, final=False,
                 /)
--

 utf_32_encode($module, str, errors=None, byteorder=0, /)
--

   utf_32_le_encode($module, str, errors=None, /)
--

             utf_32_be_encode($module, str, errors=None, /)
--

             utf_32_decode($module, data, errors=None, final=False, /)
--

  utf_32_le_decode($module, data, errors=None, final=False, /)
--

               utf_32_be_decode($module, data, errors=None, final=False, /)
--

               utf_32_ex_decode($module, data, errors=None, byteorder=0, final=False,
                 /)
--

 unicode_escape_encode($module, str, errors=None, /)
--

        unicode_escape_decode($module, data, errors=None, /)
--

       raw_unicode_escape_encode($module, str, errors=None, /)
--

    raw_unicode_escape_decode($module, data, errors=None, /)
--

   latin_1_encode($module, str, errors=None, /)
--

               latin_1_decode($module, data, errors=None, /)
--

              ascii_encode($module, str, errors=None, /)
--

 ascii_decode($module, data, errors=None, /)
--

                charmap_encode($module, str, errors=None, mapping=None, /)
--

 charmap_decode($module, data, errors=None, mapping=None, /)
--

                charmap_build($module, map, /)
--

            