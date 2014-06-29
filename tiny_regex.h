// Simplest regex for environment with restriction on code size

#ifndef TINY_REGEX_H
#define TINY_REGEX_H

#define USE_STDINT  1

#if USE_STDINT
#include <stdint.h>
#else
typedef unsigned char uint8_t;
typedef signed char int8_t;
#endif

// Add the new pattern and the result character
// Return 0 if success, -1 if error
int8_t tinreg_add_pattern(char *pat, uint8_t pat_len, char result);

// Clear all patterns
void tinreg_clear_patterns();

// Display the whole trie (for the debugging purposes)
void tinreg_display_trie();

// Rewind the position of lookup head to start
void tinreg_init_lookup();

// Forward the lookup head by one
// Return 1 if the next node exists, 0 if the next node does not exist
uint8_t tinreg_forward_lookup(char next_char);

// Return the result character, otherwise '\0'
char tinreg_get_lookup_result();

char tinreg_lookup_result(char *string);

int tinreg_pack(uint8_t **packed_data);

#endif // TINY_REGEX_H
