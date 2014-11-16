#ifndef __RAFT_CONFIG_H__
#define __RAFT_CONFIG_H__

#include "raft_callbacks.h"

typedef struct raft_config {
  uint32_t       node_count;
  raft_nodeid_t* p_nodeids;

  raft_callbacks_t cb;

  uint32_t leader_ping_interval_ms;

  uint32_t election_timeout_max_ms;
  uint32_t election_timeout_min_ms;

  uint32_t concurrency;
} raft_config_t;

#endif
