#ifndef __RAFT_STATE_H__
#define __RAFT_STATE_H__

#include "raft_types.h"

typedef struct raft_log raft_log_t;
typedef struct raft_config raft_config_t;

typedef struct raft_state {
  raft_config_t* p_config;

  raft_node_type_t type;

  /**
   * Persistent state.
   */
  struct {
    raft_nodeid_t self;
    raft_term_t   current_term;
    raft_nodeid_t voted_for;
    raft_log_t*   p_log;
  } p;

  /**
   * Volatile state.
   */
  struct {
    raft_index_t commit_index;
    raft_index_t last_applied;

    uint32_t ms_since_last_leader_ping;
    uint32_t election_timeout_ms;
  } v;

  /**
   * Leader state.
   */
  struct {
    raft_bool_t* p_ballot;

    raft_index_t* p_next_index;
    raft_index_t* p_match_index;
  } l;
} raft_state_t;

void raft_state_set_type(raft_state_t* p_state, raft_node_type_t type);

uint32_t raft_state_vote_count(raft_state_t* p_state);

#endif
