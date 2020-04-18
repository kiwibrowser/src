Please refrain from submitting a new code in this directory.
It' going to be moved to //base/sequence_manager.

Temporary base::sequence_manager namespace in Blink code will disappear soon.
In the meantime it's possible that the code is spread between scheduler/base
and //base/sequence_manager directories.

See https://crbug.com/783309 for details.
