## Code Format
  
Please sanitize the code before merging into the main branch.

### Install clang-format
 * C++: `sudo apt-get install clang-format`
 * Python: `pip install yapf`

### Formatting a file
 * C++: `clang-format -style=Google -i path/to/file`
 * C++: clang for a folder `find tools/ -iname *.h -o -iname *.cc | xargs clang-format -i`
 * Python: `yapf --style=google -i path/to/file`
 