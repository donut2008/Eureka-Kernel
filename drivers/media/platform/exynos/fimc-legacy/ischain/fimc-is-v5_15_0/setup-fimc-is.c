 descriptor.  modify($self, /, fd, eventmask)
--

Modify event mask for a registered file descriptor.

  fd
    the target file descriptor of the operation
  eventmask
    a bit set composed of the various EPOLL constants register($self, /, fd,
         eventmask=select.EPOLLIN | select.EPOLLPRI | select.EPOLLOUT)
--

Registers a new fd or raises an OSError if the fd is already registered.

  fd
    the target file descriptor of the operation
  eventmask
    a bit set composed of the various EPOLL constants

The epoll interface supports all file descriptors that support poll.        unregister($self, /, fd)
--

Remove a registered file descriptor from the epoll object.

  fd
    the target file descriptor of the operation   poll($self, /, timeout=None, maxevents=-1)
--

Wait for events on the epoll file descriptor.

  timeout
    the maximum time to wait in seconds (as float);
    a timeout of None or -1 makes poll wait indefinitely
  maxevents
    the maximum number of events returned; -1 means no limit

Returns a list containing any descriptors that have events to report,
as a list of (fd, events) 2-tuples.        __enter__($self, /)
--

        __exit__($self, exc_type=None, exc_value=None, exc_tb=None, /)
--

             This module provides an interface to the Posix calls for tty I/O control.
For a complete description of these calls, see the Posix or Unix manual
pages. It is only available for those Unix versions that support Posix
termios style tty I/O control.

All functions in this module take a file descriptor fd as their first
argument. This can be an integer file descriptor, such as returned by
sys.stdin.fileno(), or a file object, such as sys.stdin itself.            tcgetattr(fd) -> list_of_attrs

Get the tty attributes for file descriptor fd, as follows:
[iflag, oflag, cflag, lflag, ispeed, ospeed, cc] where cc is a list
of the tty special characters (each a string of length 1, except the items
with indices VMIN and VTIME, which are integers when these fields are
defined).  The interpretation of the flags and the speeds as well as the
indexing in the cc array must be done using the symbolic constants defined
in this module.             tcsetattr(fd, when, attributes) -> None

Set the tty attributes for file descriptor fd.
The attributes to be set are taken from the attributes argument, which
is a list like the one returned by tcgetattr(). The when argument
determines when the attributes are changed: termios.TCSANOW to
change immediately, termios.TCSADRAIN to change after transmitting all
queued output, or termios.TCSAFLUSH to change after transmitting all
queued output and discarding all queued input.      tcsendbreak(fd, duration) -> None

Send a break on file descriptor fd.
A zero duration sends a break for 0.25-0.5 seconds; a nonzero duration
has a system dependent meaning.   tcdrain(fd) -> None

Wait until all output written to file descriptor fd has been transmitted.  tcflush(fd, queue) -> None

Discard queued data on file descriptor fd.
The queue selector specifies which queue: termios.TCIFLUSH for the input
queue, termios.TCOFLUSH for the output queue, or termios.TCIOFLUSH for
both queues.             tcflow(fd, action) -> None

Suspend or resume input or output on file descriptor fd.
The action argument can be termios.TCOOFF to suspend output,
termios.TCOON to restart output, termios.TCIOFF to suspend input,
or termios.TCION to restart input.          This module provides various functions to manipulate time values.

There are two standard representations of time.  One is the number
of seconds since the Epoch, in UTC (a.k.a. GMT).  It may be an integer
or a floating point number (to represent fractions of seconds).
The Epoch is system-defined; on Unix, it is generally January 1st, 1970.
The actual value can be retrieved by calling gmtime(0).

The other representation is a tuple of 9 integers giving local time.
The tuple items are:
  year (including century, e.g. 1998)
  month (1-12)
  day (1-31)
  hours (0-23)
  minutes (0-59)
  seconds (0-59)
  weekday (0-6, Monday is 0)
  Julian day (day in the year, 1-366)
  DST (Daylight Savings Time) flag (-1, 0 or 1)
If the DST flag is 0, the time is given in the regular time zone;
if it is 1, the time is given in the DST time zone;
if it is -1, mktime() should guess based on the date and time.
            time() -> floating point number

Return the current time in seconds since the Epoch.
Fractions of a second may be present if the system clock provides them.    time_ns() -> int

Return the current time in nanoseconds since the Epoch.       clock_gettime(clk_id) -> float

Return the time of the specified clock clk_id.  clock_gettime_ns(clk_id) -> int

Return the time of the specified clock clk_id as nanoseconds.  clock_settime(clk_id, time)

Set the time of the specified clock clk_id.        clock_settime_ns(clk_id, time)

Set the time of the specified clock clk_id with nanoseconds.    clock_getres(clk_id) -> floating point number

Return the resolution (precision) of the specified clock clk_id. pthread_getcpuclockid(thread_id) -> int

Return the clk_id of a thread's CPU time clock.        sleep(seconds)

Delay execution for a given number of seconds.  The argument may be
a floating point number for subsecond precision.            gmtime([seconds]) -> (tm_year, tm_mon, tm_mday, tm_hour, tm_min,
                       tm_sec, tm_wday, tm_yday, tm_isdst)

Convert seconds since the Epoch to a time tuple expressing UTC (a.k.a.
GMT).  When 'seconds' is not passed in, convert the current time instead.

If the platform supports the tm_gmtoff and tm_zone, they are available as
attributes only.       localtime([seconds]) -> (tm_year,tm_mon,tm_mday,tm_hour,tm_min,
                          tm_sec,tm_wday,tm_yday,tm_isdst)

Convert seconds since the Epoch to a time tuple expressing local time.
When 'seconds' is not passed in, convert the current time instead.           asctime([tuple]) -> string

Convert a time tuple to a string, e.g. 'Sat Jun 06 16:26:11 1998'.
When the time tuple is not present, current time as returned by localtime()
is used.             ctime(seconds) -> string

Convert a time in seconds since the Epoch to a string in local time.
This is equivalent to asctime(localtime(seconds)). When the time tuple is
not present, current time as returned by localtime() is used.          mktime(tuple) -> floating point number

Convert a time tuple in local time to seconds since the Epoch.
Note that mktime(gmtime(0)) will not generally return zero for most
time zones; instead the returned value will either be equal to that
of the timezone or altzone attributes on the time module.        strftime(format[, tuple]) -> string

Convert a time tuple to a string according to a format specification.
See the library reference manual for formatting codes. When the time tuple
is not present, current time as returned by localtime() is used.

Commonly used format codes:

%Y  Year with century as a decimal number.
%m  Month as a decimal number [01,12].
%d  Day of the month as a decimal number [01,31].
%H  Hour (24-hour clock) as a decimal number [00,23].
%M  Minute as a decimal number [00,59].
%S  Second as a decimal number [00,61].
%z  Time zone offset from UTC.
%a  Locale's abbreviated weekday name.
%A  Locale's full weekday name.
%b  Locale's abbreviated month name.
%B  Locale's full month name.
%c  Locale's appropriate date and time representation.
%I  Hour (12-hour clock) as a decimal number [01,12].
%p  Locale's equivalent of either AM or PM.

Other codes may be available on your platform.  See documentation for
the C library strftime function.
       strptime(string, format) -> struct_time

Parse a string to a time tuple according to a format specification.
See the library reference manual for formatting codes (same as
strftime()).

Commonly used format codes:

%Y  Year with century as a decimal number.
%m  Month as a decimal number [01,12].
%d  Day of the month as a decimal number [01,31].
%H  Hour (24-hour clock) as a decimal number [00,23].
%M  Minute as a decimal number [00,59].
%S  Second as a decimal number [00,61].
%z  Time zone offset from UTC.
%a  Locale's abbreviated weekday name.
%A  Locale's full weekday name.
%b  Locale's abbreviated month name.
%B  Locale's full month name.
%c  Locale's appropriate date and time representation.
%I  Hour (12-hour clock) as a decimal number [01,12].
%p  Locale's equivalent of either AM or PM.

Other codes may be available on your platform.  See documentation for
the C library strftime function.
     tzset()

Initialize, or reinitialize, the local timezone to the value stored in
os.environ['TZ']. The TZ environment variable should be specified in
standard Unix timezone format as documented in the tzset man page
(eg. 'US/Eastern', 'Europe/Amsterdam'). Unknown timezones will silently
fall back to UTC. If the TZ environment variable is not set, the local
timezone is set to the systems best guess of wallclock time.
Changing the TZ environment variable without calling tzset *may* change
the local timezone used by methods such as localtime, but this behaviour
should not be relied on.    monotonic() -> float

Monotonic clock, cannot go backward.      monotonic_ns() -> int

Monotonic clock, cannot go backward, as nanoseconds.     process_time() -> float

Process time for profiling: sum of the kernel and user-space CPU time. process_time() -> int

Process time for profiling as nanoseconds:
sum of the kernel and user-space CPU time.    thread_time() -> float

Thread time for profiling: sum of the kernel and user-space CPU time.   thread_time() -> int

Thread time for profiling as nanoseconds:
sum of the kernel and user-space CPU time.      perf_counter() -> float

Performance counter for benchmarking.  perf_counter_ns() -> int

Performance counter for benchmarking as nanoseconds.  get_clock_info(name: str) -> dict

Get information of the specified clock.      Sun Mon Tue Wed Thu Fri Sat     Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec                   
                 
      
   	                   
  �           �     �          �      	 	 �  �  	  �  
           
  
     �  �        �  �  �                    �� P�  �  �  � P�  � P� P   P� �� P   �  �    �  �  �  �     
   �              