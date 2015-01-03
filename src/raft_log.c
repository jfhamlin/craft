#include <stdlib.h>
#include <string.h>

#include "raft_log.h"
#include "raft_util.h"

#define RAFT_LOG_NODE_ENTRY_COUNT \
  ((2048 - 2*sizeof(struct raft_log_node*))/sizeof(raft_log_entry_t))

typedef struct raft_log_node {
  raft_log_entry_t a_entries[RAFT_LOG_NODE_ENTRY_COUNT];
  struct raft_log_node* p_next;
} raft_log_node_t;

typedef struct raft_log {
  raft_index_t num_entries;
  raft_index_t capacity;

  raft_log_node_t* p_head;
  raft_log_node_t* p_tail;
} raft_log_t;

raft_log_t* raft_log_alloc() {
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
  p_log->p_head = p_log->p_tail = p_entry_nodes;
  p_log->num_entries = 1;

  return p_log;
}

void raft_log_free(raft_log_t* p_log) {
  if (p_log == NULL) return;

  raft_log_node_t* p_cur = p_log->p_head;
  while (p_cur) {
    for (uint32_t ii = 0; ii < RAFT_LOG_NODE_ENTRY_COUNT; ++ii) {
      free(p_cur->a_entries[ii].p_data);
    }
    raft_log_node_t* p_next = p_cur->p_next;
    free(p_cur);
    p_cur = p_next;
  }
  p_log->p_head = p_log->p_tail = NULL;
  free(p_log);
}

static raft_log_node_t* get_vacant_node(raft_log_t* p_log) {
  if (raft_log_length(p_log) % RAFT_LOG_NODE_ENTRY_COUNT != 0) {
    return p_log->p_tail;
  }

  raft_log_node_t* p_node = calloc(1, sizeof(raft_log_node_t));
  if (p_node == NULL) {
    return NULL;
  }

  p_log->p_tail->p_next = p_node;
  p_log->p_tail = p_node;

  return p_node;
}

raft_index_t raft_log_length(raft_log_t const* p_log) {
  return p_log->num_entries;
}

static raft_log_node_t* raft_log_node(raft_log_t const* p_log,
                                      raft_index_t index) {
  raft_log_node_t* p_node = p_log->p_head;
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

raft_status_t raft_log_append_user(raft_log_t* p_log,
                                   uint32_t unique_id,
                                   raft_term_t term,
                                   void* p_data,
                                   uint32_t data_size) {
  raft_log_node_t* p_node = get_vacant_node(p_log);
  if (p_node == NULL) {
    return RAFT_STATUS_OUT_OF_MEMORY;
  }

  uint32_t const index = raft_log_length(p_log) % RAFT_LOG_NODE_ENTRY_COUNT;
  raft_log_entry_t* p_entry = &p_node->a_entries[index];
  p_entry->unique_id = unique_id;
  p_entry->term = term;
  p_entry->type = RAFT_LOG_ENTRY_TYPE_USER;
  p_entry->p_data = p_data;
  p_entry->data_size = data_size;
  p_entry->replication_count = 0;

  ++p_log->num_entries;

  return RAFT_STATUS_OK;
}

raft_status_t raft_log_append(raft_log_t* p_log, raft_log_t* p_entries) {
  if (p_entries) {
    RAFT_ASSERT(RAFT_FALSE);
  }
  return RAFT_STATUS_OK;
}
