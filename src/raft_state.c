#include "string.h"

#include "raft_util.h"
#include "raft_state.h"

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
