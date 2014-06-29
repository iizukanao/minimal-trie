#include <stdio.h>
#include <string.h>
#include "tiny_regex.h"

#define USE_OSAL  0
#define ENABLE_TRIE_DIAGNOSIS  1
#define BYTES_PER_NODE  3

// USE_GRAPH==1 will not work due to a fundamental problem
#define USE_GRAPH  0

#if USE_OSAL
#include "OSAL.h"
#define MALLOC(size)  osal_mem_alloc(size)
#define CALLOC(ptr, size)  do { \
  ptr = osal_mem_alloc(size); \
  if (ptr) { \
    osal_memset(ptr, 0, size); \
  } \
} while (0)
#define MEMSET(dest, value, len)  osal_memset(dest, value, len)
#define MEMCPY(dest, src, len)  osal_memcpy(dest, src, len)
#define FREE(ptr)  osal_mem_free(ptr)
#define REALLOC(ptr, size)  do { \
  osal_mem_free(ptr); \
  ptr = osal_mem_alloc(size); \
} while (0)
#else
#include <stdlib.h>
#define MALLOC(size)  malloc(size)
#define CALLOC(ptr, size)  ptr = calloc(size, 1)
#define MEMSET(dest, value, len)  memset(dest, value, len)
#define MEMCPY(dest, src, len)  memcpy(dest, src, len)
#define REALLOC(ptr, size)  ptr = realloc(ptr, size)
#define FREE(ptr)  free(ptr)
#endif

typedef struct pnode {
  uint8_t node_char;
  char result;
  struct pnode **next_nodes;
  uint8_t num_next_nodes;
#if ENABLE_TRIE_DIAGNOSIS
  struct pnode **previous_nodes;
  uint8_t previous_nodes_len;
#endif
} pnode;

static pnode root_node = {
  .node_char = 0,
  .result = '\0',
  .next_nodes = NULL,
  .num_next_nodes = 0,
#if ENABLE_TRIE_DIAGNOSIS
  .previous_nodes = NULL,
  .previous_nodes_len = 0,
#endif
};

typedef struct pnode_stack_item {
  pnode **nodes;
  uint8_t nodes_len;
#if USE_GRAPH
  uint8_t can_merge_next;
#endif
} pnode_stack_item;

static pnode_stack_item **pnode_stack;
static uint8_t pnode_stack_len = 0;

static pnode_stack_item **pnode_group_stack;
static uint8_t pnode_group_stack_len = 0;

static pnode *lookup_head;

static int display_depth = 0;

#if USE_GRAPH
static uint8_t can_merge_next = 0;
#endif

static void add_pnode(pnode *base, pnode *add) {
  // Link base -> add
  REALLOC(base->next_nodes, sizeof(pnode *) * (base->num_next_nodes + 1));
  if (!base->next_nodes) {
    fprintf(stderr, "add_pnode: realloc failed for next_nodes: size=%lu\n",
        sizeof(pnode *) * (base->num_next_nodes + 1));
    return;
  }
  base->next_nodes[base->num_next_nodes] = add;
  base->num_next_nodes++;

#if ENABLE_TRIE_DIAGNOSIS
  // Link add -> base
  REALLOC(add->previous_nodes, sizeof(pnode *) * (add->previous_nodes_len + 1));
  if (!add->previous_nodes) {
    fprintf(stderr, "add_pnode: realloc failed for previous_nodes: size=%lu\n",
        sizeof(pnode *) * (add->previous_nodes_len + 1));
    return;
  }
  add->previous_nodes[add->previous_nodes_len] = base;
  add->previous_nodes_len++;
#endif
}

static void push_pnode_stack(pnode ***branch_nodes, uint8_t *num_branch_nodes) {
  uint8_t copy_len = sizeof(pnode *) * *num_branch_nodes;
  REALLOC(pnode_stack, sizeof(pnode *) * (pnode_stack_len + 1));
  if (!pnode_stack) {
    fprintf(stderr, "realloc failed for pnode_stack\n");
    return;
  }

  pnode_stack_item *stack_item = MALLOC(sizeof(pnode_stack_item));
  if (!stack_item) {
    fprintf(stderr, "malloc failed for pnode_stack_item\n");
    return;
  }
#if USE_GRAPH
  stack_item->can_merge_next = can_merge_next;
#endif
  stack_item->nodes = MALLOC(copy_len);
  if (!stack_item->nodes) {
    fprintf(stderr, "malloc failed for stack_item->nodes\n");
    FREE(stack_item);
    return;
  }

  MEMCPY(stack_item->nodes, *branch_nodes, copy_len);

  stack_item->nodes_len = *num_branch_nodes;

  pnode_stack[pnode_stack_len] = stack_item;
  pnode_stack_len++;

  // add to group stack
  REALLOC(pnode_group_stack, sizeof(pnode *) * (pnode_group_stack_len + 1));
  if (!pnode_group_stack) {
    fprintf(stderr, "realloc failed for pnode_group_stack\n");
    return;
  }

  pnode_stack_item *group_stack_item;
  CALLOC(group_stack_item, sizeof(pnode_stack_item));
  if (!group_stack_item) {
    fprintf(stderr, "malloc failed for group_stack_item\n");
    return;
  }
  group_stack_item->nodes_len = 0;  // TODO: unnecessary?
  pnode_group_stack[pnode_group_stack_len] = group_stack_item;
  pnode_group_stack_len++;
}

static void save_group(pnode ***branch_nodes, uint8_t *num_branch_nodes) {
  if (pnode_group_stack_len == 0) {
    fprintf(stderr, "save_group error: group stack is empty\n");
    return;
  }
  pnode_stack_item *group_stack_item = pnode_group_stack[pnode_group_stack_len - 1];
  REALLOC(group_stack_item->nodes,
      sizeof(pnode *) * (group_stack_item->nodes_len + *num_branch_nodes));
  if (!group_stack_item->nodes) {
    fprintf(stderr, "save_group error: memory allocation failed for group_stack_item\n");
    return;
  }
  uint8_t i;
  for (i = 0; i < *num_branch_nodes; i++) {
    group_stack_item->nodes[group_stack_item->nodes_len + i] = (*branch_nodes)[i];
  }
  group_stack_item->nodes_len += *num_branch_nodes;
}

static void set_head_to_last_trunk(pnode ***branch_nodes, uint8_t *num_branch_nodes) {
  if (pnode_stack_len > 0) {
    pnode_stack_item *last_trunk = pnode_stack[pnode_stack_len - 1];
#if USE_GRAPH
    can_merge_next = last_trunk->can_merge_next;
#endif
    if (*num_branch_nodes < last_trunk->nodes_len) {
      REALLOC(*branch_nodes, last_trunk->nodes_len);
      if (!branch_nodes) {
        fprintf(stderr, "set_head_to_last_trunk: failed to realloc branch_nodes\n");
        return;
      }
    }
    MEMCPY(*branch_nodes, last_trunk->nodes, sizeof(pnode *) * last_trunk->nodes_len);
    *num_branch_nodes = last_trunk->nodes_len;
  } else {
    fprintf(stderr, "set_head_to_last_trunk error: stack is empty\n");
    return;
  }
}

static int memory_usage;

static void display_node(pnode *node) {
  int i;
  int j;
  memory_usage += sizeof(pnode);
  for (j = 0; j < display_depth; j++) {
    printf("  ");
  }
  if (node->node_char != '\0') {
    printf("node %c", node->node_char);
  } else {
    printf("node (none)");
  }
#if USE_GRAPH
  printf(" (%lx)", (intptr_t)node & 0xffff);
#endif
  if (node->result != '\0') {
    printf(" (result: %c)", node->result);
  }
  printf("\n");
  for (i = 0; i < node->num_next_nodes; i++) {
    display_depth++;
    display_node(node->next_nodes[i]);
    display_depth--;
  }
}

static void merge_pnodes(pnode ***branch_nodes, uint8_t *num_branch_nodes, pnode_stack_item *add_pnodes) {
  uint8_t total_len = *num_branch_nodes + add_pnodes->nodes_len;
  REALLOC(*branch_nodes, sizeof(pnode *) * total_len);
  if (!branch_nodes) {
    fprintf(stderr, "merge_pnodes: realloc failed\n");
    return;
  }
  MEMCPY(*branch_nodes + *num_branch_nodes, add_pnodes->nodes,
      sizeof(pnode *) * add_pnodes->nodes_len);
  *num_branch_nodes += add_pnodes->nodes_len;
}

static pnode_stack_item *pop_pnode_stack(pnode ***branch_nodes, uint8_t *num_branch_nodes) {
  if (pnode_stack_len > 0) {
    pnode_group_stack_len--;
    pnode_stack_item *group_stack_item = pnode_group_stack[pnode_group_stack_len];
    if (group_stack_item->nodes_len > 0) {
      merge_pnodes(branch_nodes, num_branch_nodes, group_stack_item);
    }
    FREE(group_stack_item);

    pnode_stack_len--;
    return pnode_stack[pnode_stack_len];  // needs to be free'd in caller
  } else {
    fprintf(stderr, "pop_pnode_stack: stack is empty\n");
    return NULL;
  }
}

#if USE_GRAPH
static void delete_branch_node(pnode ***branch_nodes, uint8_t *num_branch_nodes, uint8_t delete_index) {
  uint8_t i;
  uint8_t num_deleted = 0;
  for (i = 0; i < *num_branch_nodes; i++) {
    if (i == delete_index) {
      num_deleted++;
      continue;
    }
    if (num_deleted > 0) {
      (*branch_nodes)[i-num_deleted] = (*branch_nodes)[i];
    }
  }
  (*num_branch_nodes)--;
}
#endif

static void add_branch_node(pnode ***branch_nodes, uint8_t *num_branch_nodes, uint8_t node_char, uint8_t is_optional) {
  uint8_t i, j;
  int orig_num_branch_nodes = *num_branch_nodes;
  pnode *next_node;

#if USE_GRAPH
  can_merge_next = 0;
#endif

  if (orig_num_branch_nodes == 0) {
    fprintf(stderr, "warning: branch_nodes is empty\n");
    return;
  }

#if USE_GRAPH
  if (can_merge_next) {
    CALLOC(next_node, sizeof(pnode));
    if (!next_node) {
      fprintf(stderr, "add_branch_node: calloc failed for pnode: size=%u\n",
          sizeof(pnode));
      return;
    }
    next_node->node_char = node_char;
    next_node->next_nodes = NULL;
    next_node->num_next_nodes = 0;
  }
#endif

  for (i = 0; i < orig_num_branch_nodes; i++) {
    pnode *head_node = (*branch_nodes)[i];

#if !(USE_GRAPH)
    uint8_t has_child = 0;
    // The following code is applicable only to a trie structure
    if (head_node->num_next_nodes > 0) { // check child contents
      for (j = 0; j < head_node->num_next_nodes; j++) {
        if (head_node->next_nodes[j]->node_char == node_char) {
          has_child = 1;
          (*branch_nodes)[i] = head_node->next_nodes[j];
          break;
        }
      }
    }
    if (!has_child) {
#endif

#if USE_GRAPH
      if (!can_merge_next) {
#endif
        CALLOC(next_node, sizeof(pnode));
        if (!next_node) {
          fprintf(stderr, "add_branch_node: memory allocation failed for pnode\n");
          return;
        }
        next_node->node_char = node_char;
        next_node->next_nodes = NULL;
        next_node->num_next_nodes = 0;
#if USE_GRAPH
      }
#endif
      add_pnode(head_node, next_node);
      (*branch_nodes)[i] = next_node;
#if !(USE_GRAPH)
    }  // if (!has_child)
#endif

    if (is_optional) {
      REALLOC(*branch_nodes, sizeof(pnode *) * (*num_branch_nodes + 1));
      if (!(*branch_nodes)) {
        fprintf(stderr, "realloc branch_nodes failed\n");
        return;
      }
      (*branch_nodes)[*num_branch_nodes] = head_node;
      (*num_branch_nodes)++;
    }
  }

#if USE_GRAPH
  if (can_merge_next) {
    int k;
    while (*num_branch_nodes >= 2) {
      for (j = 0; j < *num_branch_nodes; j++) {
        for (k = j+1; k < *num_branch_nodes; k++) {
          if ((*branch_nodes)[j] == (*branch_nodes)[k]) {
            delete_branch_node(branch_nodes, num_branch_nodes, j);
            goto again;
          }
        }
      }
      break;
  again:
      continue;
    }

    can_merge_next = 0;
  }

  if (is_optional) {
    can_merge_next = 1;
  }
#endif
}

#if ENABLE_TRIE_DIAGNOSIS
static void print_path_to_root(FILE *out, pnode *node) {
  char path[32];
  char *path_ptr = path;
  uint8_t depth = 0;
  while (node != &root_node) {
    if (depth >= 16) {
      fprintf(out, "depth overflow\n");
      break;
    }
    if (depth > 0) {
      *path_ptr++ = '-';
    }
    sprintf(path_ptr++, "%c", node->node_char);
    node = node->previous_nodes[0];
    depth++;
  }
  while (path_ptr != path) {
    path_ptr--;
    fprintf(out, "%c", *path_ptr);
  }
}
#endif

static void add_results(pnode **branch_nodes, uint8_t num_branch_nodes, char result) {
  uint8_t i;
  for (i = 0; i < num_branch_nodes; i++) {
    if (branch_nodes[i]->result != '\0') {
#if ENABLE_TRIE_DIAGNOSIS
      if (branch_nodes[i]->result == result) {
        fprintf(stderr, "duplicate result: %c for pattern ", result);
      } else {
        fprintf(stderr, "warning: overwriting result: %c with %c for pattern ",
            branch_nodes[i]->result, result);
      }
      print_path_to_root(stderr, branch_nodes[i]);
      fprintf(stderr, "\n");
#else
      fprintf(stderr, "warning: overwriting result: %c with %c\n",
          branch_nodes[i]->result, result);
#endif
    }
    branch_nodes[i]->result = result;
  }
}

// Add the new pattern and the result character
int8_t tinreg_add_pattern(char *pat, uint8_t pat_len, char result) {
  int i;
  pnode_stack_item *last_pnodes;
  pnode **branch_nodes = MALLOC(sizeof(pnode *));
  if (!branch_nodes) {
    fprintf(stderr, "malloc error\n");
    return -1;
  }
  branch_nodes[0] = &root_node;
  uint8_t num_branch_nodes = 1;
  for (i = 0; i < pat_len; i++) {
    char c = pat[i];
    uint8_t is_optional = 0;

    // look-ahead '?'
    if (i < pat_len - 1) {
      if (pat[i+1] == '?') {
        is_optional = 1;
        i++;
      }
    }
    switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        add_branch_node(&branch_nodes, &num_branch_nodes, c, is_optional);
        break;
      case '?':  // previous character is optional
        fprintf(stderr, "warning: orphan ? detected in pattern\n");
        // ? is already handled
        break;
      case '(':  // start grouping
        // create a branch here
        push_pnode_stack(&branch_nodes, &num_branch_nodes);
        // we don't support | without surrounding ( )
        break;
      case ')':  // end grouping
        last_pnodes = pop_pnode_stack(&branch_nodes, &num_branch_nodes);
        if (!last_pnodes) {
          fprintf(stderr, "grouping inconsistency detected at )\n");
          return -1;
        }
        if (is_optional) {
          // add popped pnodes to branch_nodes
          merge_pnodes(&branch_nodes, &num_branch_nodes, last_pnodes);
        }
        FREE(last_pnodes->nodes);
        FREE(last_pnodes);
#if USE_GRAPH
        can_merge_next = 1;
#endif

        // end branch here
        // also we have to look-ahead '?'
        break;
      case '|':  // OR
        // create a branch starting from last '('
        save_group(&branch_nodes, &num_branch_nodes);
        set_head_to_last_trunk(&branch_nodes, &num_branch_nodes);
        break;
      default:
        fprintf(stderr, "error: invalid char '%c' (only numbers allowed) in pattern: %s", c, pat);
        return -1;
    }
  }
  // add result character
  add_results(branch_nodes, num_branch_nodes, result);

  FREE(branch_nodes);

  return 0;
}

static void free_node(pnode *node) {
  uint8_t i;
  for (i = 0; i < node->num_next_nodes; i++) {
    free_node(node->next_nodes[i]);
  }
  if (node->num_next_nodes > 0) {
    FREE(node->next_nodes);
  }

#if ENABLE_TRIE_DIAGNOSIS
  // Unlink prev -> next
  for (i = 0; i < node->previous_nodes_len; i++) {
    node->previous_nodes[i]->num_next_nodes--;
  }
  if (node->previous_nodes_len > 0) {
    FREE(node->previous_nodes);
  }
#endif

  FREE(node);
}

// Clear all patterns
void tinreg_clear_patterns() {
  uint8_t i;
  for (i = 0; i < root_node.num_next_nodes; i++) {
    free_node(root_node.next_nodes[i]);
  }
  FREE(root_node.next_nodes);
  root_node.node_char = 0;
  root_node.result = '\0';
  root_node.next_nodes = NULL;
  root_node.num_next_nodes = 0;

  if (pnode_stack_len > 0) {
    fprintf(stderr, "warning: pnode_stack is not empty\n");
  }
  for (i = 0; i < pnode_stack_len; i++) {
    FREE(pnode_stack[i]->nodes);
    FREE(pnode_stack[i]);
  }
  FREE(pnode_stack);
  pnode_stack = NULL;
  pnode_stack_len = 0;

  if (pnode_group_stack_len > 0) {
    fprintf(stderr, "warning: pnode_group_stack is not empty\n");
  }
  for (i = 0; i < pnode_group_stack_len; i++) {
    FREE(pnode_group_stack[i]->nodes);
    FREE(pnode_group_stack[i]);
  }
  FREE(pnode_group_stack);
  pnode_group_stack = NULL;
  pnode_group_stack_len = 0;
}

// Display the whole trie (for the debugging purposes)
void tinreg_display_trie() {
  memory_usage = 0;
  display_node(&root_node);
  printf("---\n");
  printf("%lu nodes in total\n", memory_usage / sizeof(pnode));
}

// Rewind the position of lookup head to start
void tinreg_init_lookup() {
  lookup_head = &root_node;
}

// Forward the lookup head by one
// Return 1 if the next node exists, 0 if the next node does not exist
uint8_t tinreg_forward_lookup(char next_char) {
  uint8_t i;
  for (i = 0; i < lookup_head->num_next_nodes; i++) {
    if (lookup_head->next_nodes[i]->node_char == next_char) {
      lookup_head = lookup_head->next_nodes[i];
      return 1;  // matched
    }
  }
  return 0;  // not matched
}

// Utility function that is meant to be used for debugging and testing purposes
char tinreg_lookup_result(char *string) {
  uint8_t i, len;

  tinreg_init_lookup();
  len = strlen(string);
  for (i = 0; i < len; i++) {
    if (tinreg_forward_lookup(string[i]) != 1) {
      // lookup failed
      return '\0';
    }
  }
  return tinreg_get_lookup_result();
}

// Return the result character
char tinreg_get_lookup_result() {
  return lookup_head->result;
}

static unsigned int lookup_pos = 0;

void lookup_init_compact_string() {
  lookup_pos = 0;
}

int8_t lookup_forward_compact_string(unsigned char *compact_str, unsigned int len, char c) {
  printf("forward %c\n", c);
  unsigned int total_descendants;
#if BYTES_PER_NODE == 4
  total_descendants = (compact_str[lookup_pos+1] << 8) |
    compact_str[lookup_pos+2];
#else
  total_descendants = ((compact_str[lookup_pos] & 0xf) << 8) |
    compact_str[lookup_pos+1];
#endif
  unsigned int skipped_descendants = 0;
  if (total_descendants == 0) {
    printf("no descendants\n");
    return 0;
  }
  while (1) {
    char finding_char;
#if BYTES_PER_NODE == 4
    finding_char = compact_str[lookup_pos+BYTES_PER_NODE];
#else
    finding_char = '0' + ((compact_str[lookup_pos+BYTES_PER_NODE] & 0xf0) >> 4);
#endif
    if (finding_char == c) {
      lookup_pos += BYTES_PER_NODE;
      return 1;
    } else {
      unsigned int num_descendants;
#if BYTES_PER_NODE == 4
      num_descendants = (compact_str[lookup_pos+BYTES_PER_NODE+1] << 8) |
        compact_str[lookup_pos+BYTES_PER_NODE+2];
#else
      num_descendants = ((compact_str[lookup_pos+BYTES_PER_NODE] & 0xf) << 8) |
        compact_str[lookup_pos+BYTES_PER_NODE+1];
#endif
      if (skipped_descendants + num_descendants + 1 >= total_descendants) {
        printf("all descendants traversed\n");
        return 0;
      }
      if (lookup_pos + BYTES_PER_NODE * (num_descendants+2) >= len) { // not found
        printf("not found\n");
        return 0;
      }
      lookup_pos += BYTES_PER_NODE * (num_descendants+1);
      skipped_descendants += num_descendants + 1;
      printf("skipping %u nodes\n", num_descendants + 1);
    }
  }
}

char lookup_get_compact_string(unsigned char *compact_str) {
  return compact_str[lookup_pos + BYTES_PER_NODE - 1];
}

char lookup_compact_string(unsigned char *compact_str, unsigned int len, char *string) {
  int i;
  lookup_init_compact_string();
  for (i = 0; i < strlen(string); i++) {
    char c = string[i];
    if (lookup_forward_compact_string(compact_str, len, c) != 1) {
      printf("lookup failed\n");
      return '\0';
    }
  }
  return lookup_get_compact_string(compact_str);
}

void parse_compact_string(unsigned char *compact_str, unsigned int len) {
  int i;
  for (i = 0; i < len; i += BYTES_PER_NODE) {
#if BYTES_PER_NODE == 4
    unsigned int num_descendants = (compact_str[i+1] << 8) | compact_str[i+2];
    printf("%c-%u-%c ", compact_str[i], num_descendants, compact_str[i+3]);
#else
    char node_char = '0' + ((compact_str[i] & 0xf0) >> 4);
    unsigned int num_descendants = ((compact_str[i] & 0xf) << 8) | compact_str[i+1];
    printf("%c-%u-%c ", node_char, num_descendants, compact_str[i+2]);
#endif
  }
  printf("\n");
}

unsigned int compact_node(pnode *node, uint8_t **str, unsigned int *str_offset, unsigned int *str_capacity) {
  unsigned int this_str_offset = *str_offset;
  int i;
  unsigned int num_descendants = 0;
  for (i = 0; i < node->num_next_nodes; i++) {
    *str_offset += BYTES_PER_NODE;
    num_descendants += compact_node(node->next_nodes[i], str, str_offset, str_capacity);
  }
  if (*str_offset + BYTES_PER_NODE > *str_capacity) {
    *str = realloc(*str, *str_capacity * 2);
    if (!str) {
      fprintf(stderr, "realloc failed for str: capacity=%d\n", *str_capacity);
      return 0;
    }
    *str_capacity *= 2;
  }

#if BYTES_PER_NODE == 4
  if (num_descendants > 0xffff) {
    fprintf(stderr, "error: trie is too large (number of descendants: %d > %d)\n", num_descendants, 0xffff);
    exit(EXIT_FAILURE);
  }

  (*str)[this_str_offset] = node->node_char;
  (*str)[this_str_offset + 1] = num_descendants >> 8;
  (*str)[this_str_offset + 2] = num_descendants & 0xff;
  (*str)[this_str_offset + 3] = node->result;
#else
  if (num_descendants > 0xfff) {
    fprintf(stderr, "error: trie is too large (number of descendants: %d > %d)\n", num_descendants, 0xfff);
    exit(EXIT_FAILURE);
  }

  uint8_t node_char;
  if (node->node_char == '\0') {
    node_char = 0;
  } else {
    node_char = node->node_char - '0';
  }
  (*str)[this_str_offset] = ((node_char << 4) & 0xf0) | ((num_descendants >> 8) & 0xf);
  (*str)[this_str_offset + 1] = num_descendants & 0xff;
  (*str)[this_str_offset + 2] = node->result;
#endif

  return num_descendants + 1;
}

int tinreg_pack(uint8_t **packed_data) {
  unsigned int str_capacity = 256;
  *packed_data = malloc(str_capacity);
  if (!*packed_data) {
    fprintf(stderr, "malloc error for packed_data\n");
    return -1;
  }
  unsigned int str_offset = 0;
  int total_nodes = compact_node(&root_node, packed_data, &str_offset, &str_capacity);
  return total_nodes * BYTES_PER_NODE;
}
