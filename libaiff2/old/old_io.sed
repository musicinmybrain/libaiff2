# Old UNIX I/O to new ANSI C buffered I/O

s/\([^f]\)read( *\([^,]*\), *\([^,]*\), *\([^)]*\))/\1fread( \3, 1, \4, \2 )/g
s/\([^f]\)write( *\([^,]*\), *\([^,]*\), *\([^)]*\))/\1fwrite( \3, 1, \4, \2 )/g
s/\([^f]\)close/\1fclose/g
s/lseek/fseek/g


