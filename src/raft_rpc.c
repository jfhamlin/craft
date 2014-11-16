#include "raft_rpc.h"
#include "raft_log.h"
#include "raft_config.h"
#include "raft_state.h"
#include "raft_util.h"

static raft_status_t raft_promote_to_leader(raft_state_t* p_state);

raft_status_t
raft_recv_append_entries(raft_state_t* p_state,
                         raft_append_entries_args_t* p_args) {
  raft_log_t* p_log = p_state->p.p_log;

  if (p_args->term < p_state->p.current_term) {
    return RAFT_STATUS_INVALID_TERM;
  }

  if (p_args->term > p_state->p.current_term) {
    raft_state_set_type(p_state, RAFT_NODE_TYPE_FOLLOWER);
    p_state->p.current_term = p_args->term;
  } else if (p_state->type == RAFT_NODE_TYPE_LEADER) {
    RAFT_LOG(p_state,
             "Leader received invalid AppendEntries request for the current"
             " term from another leader. Sender id: %llu.",
             p_args->leader_id);
    return RAFT_STATUS_INVALID_ARGS;
  }

  p_state->v.heard_from_leader = RAFT_TRUE;

  raft_log_entry_t const* p_prev_entry;
  p_prev_entry = raft_log_entry(p_log, p_args->prev_log_index);
  if (p_prev_entry == NULL || p_prev_entry->term != p_args->prev_log_term) {
    // TODO: Delete unexpected entries. Enqueue response.
    return RAFT_STATUS_OK;
  }

  raft_log_append(p_state->p.p_log, p_args->p_log_entries);

  p_state->v.commit_index = MIN(MAX(p_state->v.commit_index,
                                    p_args->leader_commit),
                                raft_log_length(p_log) - 1);

  // TODO: Enqueue response.
  return RAFT_STATUS_OK;
}

raft_status_t
raft_recv_append_entries_response(raft_state_t* p_state,
                                  raft_append_entries_response_args_t* p_args) {
  return RAFT_STATUS_OK;
}

raft_status_t
raft_recv_request_vote(raft_state_t* p_state,
                       raft_request_vote_args_t* p_args) {
  RAFT_LOG(p_state, "Received vote from nodeid: %llu, term: %u",
           p_args->candidate_id, p_args->term);

  raft_request_vote_response_args_t response = {
    .follower_id = p_state->p.self,
    .vote_granted = RAFT_FALSE,
    .term = p_state->p.current_term
  };

  if (p_args->term < p_state->p.current_term) {
    goto respond;
  }

  if (p_args->term > p_state->p.current_term) {
    response.term = p_state->p.current_term = p_args->term;
    raft_state_set_type(p_state, RAFT_NODE_TYPE_FOLLOWER);
    p_state->p.voted_for = 0;
  }

  if (p_state->p.voted_for && p_state->p.voted_for != p_args->candidate_id) {
    goto respond;
  }

  raft_log_t const* p_log = p_state->p.p_log;
  raft_log_entry_t const* p_entry = raft_log_entry(p_log, -1);
  raft_term_t const latest_term = p_entry ? p_entry->term : 0;
  if (p_args->last_log_term > latest_term ||
      (p_args->last_log_term == latest_term &&
       p_args->last_log_index + 1 >= raft_log_length(p_log))) {

    RAFT_LOG(p_state, "Voting for nodeid: %llu, term: %u.",
             p_args->candidate_id, p_args->term);
    p_state->p.voted_for = p_args->candidate_id;

    response.vote_granted = RAFT_TRUE;
    goto respond;
  }

respond:

  p_state->p_config->cb.p_request_vote_response_rpc(
      p_args->candidate_id, &response);

  return RAFT_STATUS_OK;
}

raft_status_t
raft_recv_request_vote_response(raft_state_t* p_state,
                                raft_request_vote_response_args_t* p_args) {
  /** VALIDATIONS **/
  if (p_args->term != p_state->p.current_term) {
    if (p_args->term > p_state->p.current_term) {
      RAFT_LOG(p_state,
               "Invalid request vote response. Response term %d > current %d.",
               p_args->term, p_state->p.current_term);
      return RAFT_STATUS_INVALID_TERM;
    }

    return RAFT_STATUS_OK;
  }

  if (p_state->type == RAFT_NODE_TYPE_FOLLOWER) {
    RAFT_LOG(p_state,
             "Received request vote response for the current term, "
             "but this node is just a follower!");
    return RAFT_STATUS_INVALID_ARGS;
  }

  uint32_t const node_count = p_state->p_config->node_count;
  if (p_args->follower_id > node_count) {
    RAFT_LOG(p_state, "Received request vote response from unknown node.");
    return RAFT_STATUS_INVALID_ARGS;
  }

  if (p_state->type == RAFT_NODE_TYPE_LEADER) {
    return RAFT_STATUS_OK;
  }

  /* Record and count votes! */
  raft_bool_t* p_ballot = p_state->l.p_ballot;
  p_ballot[p_args->follower_id - 1] = p_args->vote_granted;

  uint32_t sum = 0;
  for (uint32_t i = 0; i < node_count; ++i) {
    if (p_ballot[i])
      ++sum;
  }

  if (sum > node_count / 2) {
    raft_promote_to_leader(p_state);
  }

  return RAFT_STATUS_OK;
}

static raft_status_t raft_promote_to_leader(raft_state_t* p_state) {
  RAFT_LOG(p_state, "Leader for term %u.",
           p_state->p.current_term);

  raft_state_set_type(p_state, RAFT_NODE_TYPE_LEADER);

  /* Send initial AppendEntries messages to establish leadership. */
  uint32_t const node_count = p_state->p_config->node_count;
  for (uint32_t i = 0; i < node_count; ++i) {

  }

  return RAFT_STATUS_OK;
}
