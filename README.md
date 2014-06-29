minimal-trie is a C library to search a path from a trie with a minimal compiled code size in exchange for a limited feature set. Suitable for embedded systems.

# Limitations

- Only digits (0..9) can be used as a pattern
- Only single char can be used as a result

# How to use

## Preparation

### Define a trie with regular expressions

Write a pattern file with the following format:

    <regex-pattern><spaces-or-tabs><result>
    <regex-pattern><spaces-or-tabs><result>
    ...

You can use only digits (0-9) as the pattern character, and you can use only single char as the result. In this guide, we refer to this file as patterns.txt.

Example of patterns.txt:

    41?3     a
    (12|21)3 b
    32       c

Trie data is generated from patterns.txt. The above patterns.txt define the following pairs of paths and results.

    path  -> result
    4-1-3 -> a
    4-3   -> a
    1-2-3 -> b
    2-1-3 -> b
    3-2   -> c

### Regular expressions

Very limited set of regular expressions are supported. Available special characters are `?`, `|`, and `( )`. `|` can't be used without surrounding `( )`. You can use only digits (0-9) as normal characters. '^' and '$' are automatically inserted before and after each pattern.

Examples of regular expressions:

    regex        -> matches to
    1234         -> 1-2-3-4
    1(2|3)4      -> 1-2-4, 1-3-4
    12?3         -> 1-2-3, 1-3
    1(23)?4      -> 1-2-3-4, 1-4
    (1(2|3)?4)?5 -> 1-2-4-5, 1-3-4-5, 1-4-5, 5

### Building the trie data

Run `make` to build the program "build_trie".

    $ make

Run build_trie with the pattern filename as the argument. Save the output as trie_data.h.

    $ ./build_trie patterns.txt > trie_data.h

To check the parse result, run build_trie with `-s` option:

    $ ./build_trie -s patterns.txt
    node (none)
      node 4
        node 1
          node 3 (result: a)
        node 3 (result: a)
      node 1
        node 2
          node 3 (result: b)
      node 2
        node 1
          node 3 (result: b)
      node 3
        node 2 (result: c)
    ---
    13 nodes in total

# Searching

Put trie_data.h, minimal_trie.h, and minimal_trie.c in the include path of your project.

    #include <stdio.h>

    // Include the library
    #include "minimal_trie.h"

    // Load the trie data into the variable trie_data
    #include "trie_data.h"

    int main(void) {
      char c;

      // Initial setup
      trie_set_data(trie_data, sizeof(trie_data));

      // Perform the first search
      trie_start();
      // Find the path 4-1-3 from the trie
      trie_forward(4);
      trie_forward(1);
      trie_forward(3);
      c = trie_get_result();
      if (c != '\0') {
        printf("found: %c\n", c);
      } else {
        printf("not found\n");
      }

      // Perform the second search
      trie_start();
      // Find the path 2-4 from the trie
      trie_forward(2);
      trie_forward(4);
      c = trie_get_result();
      if (c != '\0') {
        printf("found: %c\n", c);
      } else {
        printf("not found\n");
      }

      return 0;
    }

Save the above code as search.c. To compile the program with gcc, run:

    $ gcc -o search search.c minimal_trie.c
    $ ./search
    found: a
    not found

