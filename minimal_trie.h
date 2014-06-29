#ifndef MINIMAL_TRIE_H
#define MINIMAL_TRIE_H

#define BYTES_PER_NODE  3
#define USE_STDINT  1

#if USE_STDINT
#include <stdint.h>
#else
typedef unsigned char uint8_t;
typedef signed char int8_t;
#endif

// Set trie data
void trie_set_data(uint8_t *data, unsigned int len);

// Initialize the search (set root as the current node)
void trie_start();

// Go down one node
// Only 4 bit values (0-15) are allowed as a next_char
int8_t trie_forward(uint8_t next_char);

// Get the result for the current node
uint8_t trie_get_result();

#endif // MINIMAL_TRIE_H
