// Library for looking up a result in trie data structure

#include "minimal_trie.h"

static unsigned int lookup_pos = 0;
static uint8_t *trie_data;
static unsigned int trie_data_len;

// Set trie data
void trie_set_data(uint8_t *data, unsigned int len) {
  trie_data = data;
  trie_data_len = len;
}

// Start the search (set root as the current node)
void trie_start() {
  lookup_pos = 0;
}

// Go down one node
int8_t trie_forward(uint8_t next_char) {
  unsigned int total_descendants;
  total_descendants = ((trie_data[lookup_pos] & 0xf) << 8) |
    trie_data[lookup_pos+1];
  unsigned int skipped_descendants = 0;
  if (total_descendants == 0) {
    // no descendants
    return 0;
  }
  while (1) {
    if (((trie_data[lookup_pos+BYTES_PER_NODE] & 0xf0) >> 4) == next_char) {
      lookup_pos += BYTES_PER_NODE;
      return 1;
    } else {
      unsigned int num_descendants;
      num_descendants = ((trie_data[lookup_pos+BYTES_PER_NODE] & 0xf) << 8) |
        trie_data[lookup_pos+BYTES_PER_NODE+1];
      if (skipped_descendants + num_descendants + 1 >= total_descendants) {
        // all descendants have been traversed
        return 0;
      }
      if (lookup_pos + BYTES_PER_NODE * (num_descendants+2) >= trie_data_len) {
        // not found
        return 0;
      }
      lookup_pos += BYTES_PER_NODE * (num_descendants+1);
      // skip nodes
      skipped_descendants += num_descendants + 1;
    }
  }
}

// Get the result for the current node
uint8_t trie_get_result() {
  return trie_data[lookup_pos + BYTES_PER_NODE - 1];
}
