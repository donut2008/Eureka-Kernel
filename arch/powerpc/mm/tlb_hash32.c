F �F �F �F �F �F vF 	F F F F F F F F F �F �F �F F F F F F �F F F F F TF F F F F F F F F F F F F F F F F F &F cF � F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F �F "F �F �F �F �F �F �F �F �F RF �F �F �F �F , F �F �F x F � F �6F �4F �6F �6F �6F �6F �6F �6F �6F �6F �3F �LF �LF �LF �LF �MF �MF |NF �NF 9RF 9RF 9RF 9RF MF YOF &MF (PF 9RF �LF 4QF 9RF 9RF �LF 9RF 9RF PQF �MF 9RF �MF MF &MF �LF �LF MF &MF �LF �LF MF &MF �LF �LF RF *RF GRF SF SF SF SF SF �RF SF SF SF SF �RF �QF SF SF SF SF SF SF SF SF SF SF SF SF SF SF SF SF �QF          SRE 2.2.2 Copyright (c) 1997-2002 by Secret Labs AB            Compiled regular expression object.             match($self, /, string, pos=0, endpos=sys.maxsize)
--

Matches zero or more characters at the beginning of the string.          fullmatch($self, /, string, pos=0, endpos=sys.maxsize)
--

Matches against all of the string.   search($self, /, string, pos=0, endpos=sys.maxsize)
--

Scan through string looking for a match, and return a corresponding match object instance.

Return None if no position in the string matches.           sub($self, /, repl, string, count=0)
--

Return the string obtained by replacing the leftmost non-overlapping occurrences of pattern in string by the replacement repl.         subn($self, /, repl, string, count=0)
--

Return the tuple (new_string, number_of_subs_made) found by replacing the leftmost non-overlapping occurrences of pattern with the replacement repl.  findall($self, /, string, pos=0, endpos=sys.maxsize)
--

Return a list of all non-overlapping matches of pattern in string.     split($self, /, string, maxsplit=0)
--

Split string by the occurrences of pattern.             finditer($self, /, string, pos=0, endpos=sys.maxsize)
--

Return an iterator over all non-overlapping matches for the RE pattern in string.

For each match, the iterator returns a match object.               scanner($self, /, string, pos=0, endpos=sys.maxsize)
--

       __copy__($self, /)
--

         __deepcopy__($self, memo, /)
--

               The result of re.match() and re.search().
Match objects always have a boolean value of True.    group([group1, ...]) -> str or tuple.
    Return subgroup(s) of the match by indices or names.
    For 0 returns the entire match.              start($self, group=0, /)
--

Return index of the start of the substring matched by group.       end($self, group=0, /)
--

Return index of the end of the substring matched by group.           span($self, group=0, /)
--

For match object m, return the 2-tuple (m.start(group), m.end(group)).              groups($self, /, default=None)
--

Return a tuple containing all the subgroups of the match, from 1.

  default
    Is used for groups that did not participate in the match.   groupdict($self, /, default=None)
--

Return a dictionary containing all the named subgroups of the match, keyed by the subgroup name.

  default
    Is used for groups that did not participate in the match. expand($self, /, template)
--

Return the string obtained by doing backslash substitution on the string template, as done by the sub() method.  __copy__($self, /)
--

         __deepcopy__($self, memo, /)
--

               match($self, /)
--

            search($self, /)
--

           compile($module, /, pattern, flags, code, groups, groupindex,
        indexgroup)
--

          getcodesize($module, /)
--

    ascii_iscased($module, character, /)
--

       unicode_iscased($module, character, /)
--

     ascii_tolower($module, character, /)
--

       unicode_tolower($module, character, /)
--

     0�F ��F ��F 0�F h�F 0�F ��F �F ��F ��F ��F ��F ��F �F �F �F �F �F �F �F �F �F �F ŨF ըF �F ��F q�F X�F a�F ��F w�F ��F �F ��F ��F ��F             A certificate could not be verified.            SSL/TLS session closed cleanly. Non-blocking SSL socket needs to read more data
before the requested operation can be completed.                Non-blocking SSL socket needs to write more data
before the requested operation can be completed.               System error when attempting SSL operation.     SSL/TLS connection terminated abruptly.         _wrap_socket($self, /, sock, server_side, server_hostname=None, *,
             owner=None, session=None)
--

  _wrap_bio($self, /, incoming, outgoing, server_side,
          server_hostname=None, *, owner=None, session=None)
--

          set_ciphers($self, cipherlist, /)
--

          _set_alpn_protocols($self,