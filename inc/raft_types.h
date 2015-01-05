#ifndef __RAFT_TYPES_H__
#define __RAFT_TYPES_H__

#include <stdint.h>

typedef _Bool raft_bool_t;
#define RAFT_TRUE  1
#define RAFT_FALSE 0

typedef enum {
  RAFT_NODE_TYPE_LEADER,
  RAFT_NODE_TYPE_FOLLOWER,
  RAFT_NODE_TYPE_CANDIDATE,
} raft_node_type_t;

typedef enum {
  RAFT_STATUS_OK,
  RAFT_STATUS_OUT_OF_MEMORY,
  RAFT_STATUS_INVALID_TERM,
  RAFT_STATUS_INVALID_ARGS,
  RAFT_STATUS_INVALID_MESSAGE,
} raft_status_t;

#define RAFT_SUCCESS(_status) ((_status) == RAFT_STATUS_OK)
#define RAFT_FAILURE(_status) ((_status) != RAFT_STATUS_OK)

typedef uint32_t raft_term_t;
typedef uint32_t raft_nodeid_t;
typedef uint32_t raft_index_t;

struct raft_log_t;

#endif
