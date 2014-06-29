CC=cc
CFLAGS=-Wall
SOURCES=tiny_regex.c build_trie.c
HEADERS=tiny_regex.h
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=build_trie

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) -c $< -o $@ $(CFLAGS)

test: all
	@$(MAKE) -C test

.PHONY: test clean

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)
	@$(MAKE) -w -C test clean
