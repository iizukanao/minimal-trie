CC=cc
CFLAGS=-Wall

all: trie_search_test

trie_test_data.h: patterns.txt ../../build_trie
	../../build_trie patterns.txt > trie_test_data.h 2>/dev/null

../../build_trie:
	@$(MAKE) -C ../..

trie_search_test.o: trie_search_test.c trie_test_data.h
	$(CC) -c -I../.. -o trie_search_test.o trie_search_test.c

trie_search_test: trie_search_test.o ../../minimal_trie.o
	$(CC) $(LDFLAGS) -o trie_search_test trie_search_test.o ../../minimal_trie.o

../../minimal_trie.o: ../../minimal_trie.h ../../minimal_trie.c
	$(CC) -c -o ../../minimal_trie.o ../../minimal_trie.c

.PHONY: clean

clean:
	rm -f trie_search_test trie_search_test.o trie_test_data.h
