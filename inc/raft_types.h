#ifndef __RAFT_TYPES_H__
#define __RAFT_TYPES_H__

#include <stdint.h>

typedef enum {
  RAFT_NODE_TYPE_LEADER,
  RAFT_NODE_TYPE_FOLLOWER,
  RAFT_NODE_TYPE_CANDIDATE,
} raft_node_type_t;

typedef enum {
  RAFT_STATUS_OK,
  RAFT_STATUS_INVALID_TERM,
  RAFT_STATUS_INVALID_ARGS,
} raft_status_t;

typedef _Bool raft_bool_t;
#define RAFT_TRUE  1
#define RAFT_FALSE 0

typedef uint32_t raft_term_t;
typedef uint64_t raft_nodeid_t;
typedef uint32_t raft_index_t;

struct raft_log_t;

#endif
