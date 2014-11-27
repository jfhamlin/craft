#include <stdlib.h>
#include <string.h>

#include "raft_log.h"
#include "raft_util.h"

#define RAFT_LOG_NODE_ENTRY_COUNT \
  (1024 - sizeof(struct raft_log_node*))/sizeof(raft_log_entry_t)

typedef struct raft_log_node {
  raft_log_entry_t a_entries[RAFT_LOG_NODE_ENTRY_COUNT];
  struct raft_log_node* p_next;
} raft_log_node_t;

typedef struct raft_log {
  raft_index_t num_entries;
  raft_index_t capacity;

  raft_log_node_t* p_entry_nodes;
} raft_log_t;

raft_log_t* raft_log_create() {
  raft_log_t* p_log = calloc(1, sizeof(raft_log_t));
  if (p_log == NULL) {
    return NULL;
  }

  raft_log_node_t* p_entry_nodes = calloc(1, sizeof(raft_log_node_t));
  if (p_entry_nodes == NULL) {
    free(p_log);
    return NULL;
  }

  p_entry_nodes->a_entries[0].type = RAFT_LOG_ENTRY_TYPE_SYSTEM;
  p_log->p_entry_nodes = p_entry_nodes;
  p_log->num_entries = 1;

  return p_log;
}

void raft_log_free(raft_log_t* p_log) {

}

raft_index_t raft_log_length(raft_log_t const* p_log) {
  return p_log->num_entries;
}

static raft_log_node_t* raft_log_node(raft_log_t const* p_log,
                                      raft_index_t index) {
  raft_log_node_t* p_node = p_log->p_entry_nodes;
  while (index > 0) {
    RAFT_ASSERT(p_node);
    p_node = p_node->p_next;
  }
  return p_node;
}

raft_log_entry_t const* raft_log_entry(raft_log_t const* p_log, int32_t index) {
  if (index < 0) {
    index = p_log->num_entries + index;
  }

  RAFT_ASSERT_STR(index >= 0 && index < p_log->num_entries,
                  "index: %d", index);

  raft_index_t node_index = index / RAFT_LOG_NODE_ENTRY_COUNT;
  raft_log_node_t* p_node = raft_log_node(p_log, node_index);

  RAFT_ASSERT(p_node);

  return &p_node->a_entries[index % RAFT_LOG_NODE_ENTRY_COUNT];
}

raft_status_t raft_log_append(raft_log_t* p_log, raft_log_t* p_entries) {
  RAFT_ASSERT(RAFT_FALSE);
  return RAFT_STATUS_OK;
}
