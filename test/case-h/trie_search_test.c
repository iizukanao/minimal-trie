#include <stdio.h>
#include <assert.h>

#include "minimal_trie.h"
#include "trie_test_data.h"

int main() {
  trie_set_data(trie_data, sizeof(trie_data));

  trie_start();
  assert(trie_forward(1) == 1);
  assert(trie_forward(2) == 1);
  assert(trie_forward(3) == 1);
  assert(trie_forward(7) == 1);
  assert(trie_get_result() == 'h');

  trie_start();
  assert(trie_forward(1) == 1);
  assert(trie_forward(4) == 1);
  assert(trie_forward(5) == 1);
  assert(trie_forward(7) == 0);

  trie_start();
  assert(trie_forward(1) == 1);
  assert(trie_forward(4) == 1);
  assert(trie_forward(5) == 1);
  assert(trie_forward(6) == 1);
  assert(trie_forward(7) == 1);
  assert(trie_get_result() == 'h');

  trie_start();
  assert(trie_forward(1) == 1);
  assert(trie_forward(4) == 1);
  assert(trie_forward(6) == 1);
  assert(trie_forward(7) == 1);
  assert(trie_get_result() == 'h');

  trie_start();
  assert(trie_forward(1) == 1);
  assert(trie_forward(7) == 1);
  assert(trie_get_result() == 'h');

  return 0;
}
