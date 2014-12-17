#include "string.h"

#include "raft_util.h"
#include "raft_state.h"
#include "raft_config.h"

static char* a_type_strings[] = {
  "RAFT_NODE_TYPE_LEADER",
  "RAFT_NODE_TYPE_FOLLOWER",
  "RAFT_NODE_TYPE_CANDIDATE",
};

void raft_state_set_type(raft_state_t* p_state, raft_node_type_t type) {
  if (p_state->type != type) {
    RAFT_LOG(p_state, "%s -> %s.",
             a_type_strings[p_state->type], a_type_strings[type]);
  }
  p_state->type = type;
}

uint32_t raft_state_vote_count(raft_state_t* p_state) {
  uint32_t const node_count = p_state->p_config->node_count;
  raft_bool_t const* p_ballot = p_state->l.p_ballot;
  uint32_t sum = 0;
  for (uint32_t i = 0; i < node_count; ++i) {
    if (p_ballot[i])
      ++sum;
  }

  return sum;
}
