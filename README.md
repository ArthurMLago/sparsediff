# Sparsediff
A simple application to diff two sparse files.

Instead of reading the entire file with `read` system calls, we skip holes and look for data using `lseek`, 
`SEEK_DATA` and `SEEK_HOLE`.

