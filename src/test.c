#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raft.h"
#include "raft_state.h"
#include "raft_config.h"
#include "raft_log.h"
#include "raft_rpc.h"

#define NODE_COUNT 5

raft_state_t* states[NODE_COUNT];

static
raft_status_t
append_entries_rpc(raft_nodeid_t id, raft_append_entries_args_t* args) {
  return raft_recv_append_entries(states[id - 1], args);
}

static
raft_status_t
append_entries_response_rpc(raft_nodeid_t id,
                            raft_append_entries_response_args_t* args) {
  return raft_recv_append_entries_response(states[id - 1], args);
}

static
raft_status_t
request_vote_rpc(raft_nodeid_t id, raft_request_vote_args_t* args) {
  return raft_recv_request_vote(states[id - 1], args);
}

static
raft_status_t
request_vote_response_rpc(raft_nodeid_t id,
                          raft_request_vote_response_args_t* args) {
  return raft_recv_request_vote_response(states[id - 1], args);
}

raft_state_t* make_raft_state(uint32_t id) {
  raft_state_t* p_state = calloc(1, sizeof(raft_state_t));

  p_state->type = RAFT_NODE_TYPE_FOLLOWER;
  p_state->p.self = id;

  p_state->p.p_log = raft_log_create();

  raft_config_t* p_config = calloc(1, sizeof(raft_config_t));
  p_state->p_config = p_config;

  p_config->node_count = NODE_COUNT;
  p_config->p_nodeids = calloc(1, (NODE_COUNT - 1) * sizeof(raft_nodeid_t));

  raft_nodeid_t other_id = 1;
  for (uint32_t i = 0; i < NODE_COUNT - 1; ++i) {
    if (other_id == id) ++other_id;
    p_config->p_nodeids[i] = other_id++;
  }

  p_config->election_timeout_max_ms = 5000;
  p_config->election_timeout_min_ms = 4000;

  p_config->cb.pf_append_entries_rpc =          &append_entries_rpc;
  p_config->cb.pf_append_entries_response_rpc = &append_entries_response_rpc;
  p_config->cb.pf_request_vote_rpc =            &request_vote_rpc;
  p_config->cb.pf_request_vote_response_rpc =   &request_vote_response_rpc;

  uint32_t const ballot_size = NODE_COUNT * sizeof(raft_bool_t);
  p_state->l.p_ballot = calloc(1, ballot_size);

  return p_state;
}


int main(int argc, char** argv) {
  memset(&states[0], 0, sizeof(states));
  for (uint32_t i = 0; i < NODE_COUNT; ++i) {
    states[i] = make_raft_state(i + 1);
  }

  for (;;);

  return 0;
}
