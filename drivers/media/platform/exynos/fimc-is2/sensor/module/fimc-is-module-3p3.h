ailable, using it will raise a NotImplementedError.

The mode argument is ignored on Windows. nice($module, increment, /)
--

Add increment to the priority of process and return the new priority.           getpriority($module, /, which, who)
--

Return program scheduling priority.     setpriority($module, /, which, who, priority)
--

Set program scheduling priority.              posix_spawn($module, path, argv, env, /, *, file_action