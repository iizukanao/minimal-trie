#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include "tiny_regex.h"

void print_usage() {
  printf("Usage: build_trie [options] <pattern_file>\n");
  printf("\n");
  printf("Options:\n");
  printf("  -s, --showtrie   show the result trie\n");
}

int main(int argc, char **argv) {
  FILE *fp;
  char buf[1024];
  int opt_showtrie = 0;

  static struct option long_options[] = {
    { "showtrie", no_argument, NULL, 's' },
    { 0, 0, 0, 0 },
  };
  int option_index = 0;
  int opt;
  while ((opt = getopt_long(argc, argv, "s", long_options, &option_index)) != -1) {
    switch (opt) {
      case 's':
        opt_showtrie = 1;
        break;
      default:
        print_usage();
        return EXIT_FAILURE;
    }
  }

  if (argc < optind + 1) {
    print_usage();
    return EXIT_FAILURE;
  }

  fp = fopen(argv[optind], "r");
  if (!fp) {
    fprintf(stderr, "Error opening %s: %s", argv[optind], strerror(errno));
    return EXIT_FAILURE;
  }

  int line_count = 0;
  while (fgets(buf, 1024, fp)) {
    line_count++;
    int pattern_len = 0;
    int is_space_found = 0;
    char result = '\0';
    int i;
    for (i = 0; i < strlen(buf); i++) {
      if (buf[i] == '\n') {
        break;
      }
      if (buf[i] == ' ' || buf[i] == '\t') {
        if (!is_space_found) {
          is_space_found = 1;
        }
      } else {
        if (is_space_found) {
          if (result == '\0') {
            result = buf[i];
          } else {
            fprintf(stderr, "syntax error at line %d (result must be single char): %s",
                line_count, buf);
            return EXIT_FAILURE;
          }
        }
      }
      if (!is_space_found) {
        pattern_len++;
      }
    }
    if (pattern_len == 0 && result == '\0') { // empty line
      continue;
    } else if (pattern_len == 0 || result == '\0' || !is_space_found) { // syntax error
      fprintf(stderr, "syntax error at line %d: %s", line_count, buf);
      fprintf(stderr, "correct format is \"<regex_pattern> <result>\"\n");
      return EXIT_FAILURE;
    }
    if (tinreg_add_pattern(buf, pattern_len, result) != 0) {
      return EXIT_FAILURE;
    }
  }

  if (opt_showtrie) {
    tinreg_display_trie();
  } else {
    uint8_t *packed_data;
    int packed_data_len;
    int i;
    packed_data_len = tinreg_pack(&packed_data);
    printf("static uint8_t trie_data[] = {\n");
    for (i = 0; i < packed_data_len; i++) {
      if (i % 8 == 0) {
        if (i != 0) {
          printf("\n");
        }
        printf("  ");
      } else {
        printf(" ");
      }
      printf("0x%02x,", packed_data[i]);
    }
    printf("\n};  // %d bytes\n", packed_data_len);
    free(packed_data);
  }

  fclose(fp);

  return EXIT_SUCCESS;
}
