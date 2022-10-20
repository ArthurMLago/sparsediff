# Sparsediff
A simple application to diff two sparse files.

Instead of reading the entire file with `read` system calls, we skip holes and look for data using `lseek`, 
`SEEK_DATA` and `SEEK_HOLE`.

## Usage

./sparsediff <file 1> <file 1> [-c|--color-diffs] [-m|--data-marker] [-l|--context-lines <n. lines>]

 - -c or --color-diffs colors differences in red
 - -m or --data-marker prints a header for each "page" of data found
 - -l or --context-lines sets how many lines of context before and after a difference are printed

