#include <stdio.h>
#include <assert.h>

#include "minimal_trie.h"
#include "trie_test_data.h"

int main() {
  trie_set_data(trie_data, sizeof(trie_data));

  trie_start();
  assert(trie_get_result() == 'A');
  assert(trie_forward(0) == 1);
  assert(trie_get_result() == 'A');
  assert(trie_forward(1) == 0);

  trie_start();
  assert(trie_get_result() == 'A');
  assert(trie_forward(1) == 1);
  assert(trie_get_result() == 'A');
  assert(trie_forward(1) == 0);

  trie_start();
  assert(trie_get_result() == 'A');
  assert(trie_forward(2) == 1);
  assert(trie_get_result() == 'A');
  assert(trie_forward(1) == 0);

  trie_start();
  assert(trie_get_result() == 'A');
  assert(trie_forward(3) == 1);
  assert(trie_get_result() == 'A');
  assert(trie_forward(1) == 0);

  trie_start();
  assert(trie_get_result() == 'A');
  assert(trie_forward(9) == 1);
  assert(trie_get_result() == '\0');
  assert(trie_forward(8) == 1);
  assert(trie_get_result() == '\0');
  assert(trie_forward(7) == 1);
  assert(trie_get_result() == '\0');
  assert(trie_forward(6) == 1);
  assert(trie_get_result() == '\0');
  assert(trie_forward(5) == 1);
  assert(trie_get_result() == '\0');
  assert(trie_forward(4) == 1);
  assert(trie_get_result() == '\0');
  assert(trie_forward(3) == 1);
  assert(trie_get_result() == '\0');
  assert(trie_forward(2) == 1);
  assert(trie_get_result() == '\0');
  assert(trie_forward(1) == 1);
  assert(trie_get_result() == '\0');
  assert(trie_forward(0) == 1);
  assert(trie_get_result() == 'B');
  assert(trie_forward(0) == 0);

  return 0;
}
