#ifndef __RAFT_LOG_H__
#define __RAFT_LOG_H__

#include "raft_types.h"

typedef struct raft_log_entry {
  uint32_t unique_id;
  raft_term_t term;

  enum {
    RAFT_LOG_ENTRY_TYPE_USER,
    RAFT_LOG_ENTRY_TYPE_SYSTEM,
  } type;

  void*  p_data;
  uint32_t data_size;

  uint32_t replication_count;
} raft_log_entry_t;

typedef struct raft_log raft_log_t;

raft_log_t* raft_log_alloc();

void raft_log_free(raft_log_t* p_log);

uint32_t raft_log_length(raft_log_t const* p_log);

raft_log_entry_t const* raft_log_entry(raft_log_t const* p_log, int32_t index);

raft_status_t raft_log_append_user(raft_log_t* p_log,
                                   uint32_t unique_id,
                                   raft_term_t term,
                                   void* p_data,
                                   uint32_t data_size);

raft_status_t raft_log_append(raft_log_t* p_log, raft_log_t* p_entries);

#endif
