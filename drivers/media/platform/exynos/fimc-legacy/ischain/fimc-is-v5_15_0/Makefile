e a proxy object that weakly
references 'object'.  'callback', if given, is called with a
reference to the proxy when 'object' is about to be finalized.      A channel ID identifies a channel and may be used as an int.    This module provides primitive operations to manage Python interpreters.
The 'interpreters' module provides a more convenient interface.        create() -> ID

Create a new interpreter and return a unique generated ID.      destroy(id)

Destroy the identified interpreter.

Attempting to destroy the current interpreter results in a RuntimeError.
So does an unrecognized ID.          list_all() -> [ID]

Return a list containing the ID of every existing interpreter.              get_current() -> ID

Return the ID of current interpreter.      get_main() -> ID

Return the ID of main interpreter.            is_running(id) -> bool

Return whether or not the identified interpreter is running.            run_string(id, script, shared)

Execute the provided string in the identified interpreter.

See PyRun_SimpleStrings.            is_shareable(obj) -> bool

Return True if the object's data may be shared between interpreters and
False otherwise.             channel_create() -> cid

Create a new cross-interpreter channel and return a unique generated ID.               channel_destroy(cid)

