#include <stdlib.h>

#include "raft_rpc.h"
#include "raft_log.h"
#include "raft_config.h"
#include "raft_state.h"
#include "raft_util.h"
#include "raft_wire.h"

static raft_status_t promote_to_leader(raft_state_t* p_state);
static void on_leader_ping(raft_state_t* p_state);

static raft_status_t send_request_vote_response(
    raft_state_t* p_state,
    raft_nodeid_t recipient_id,
    raft_request_vote_response_args_t* p_args);


raft_status_t raft_recv_message(raft_state_t* p_state,
                                void* p_message_bytes,
                                uint32_t buffer_size) {
  raft_status_t status = RAFT_STATUS_OK;
  /* TODO: Validate message version number */
  switch (raft_message_type(p_message_bytes)) {
    case MSG_TYPE_REQUEST_VOTE:
    {
      raft_request_vote_args_t args;
      status = raft_read_request_vote_args(&args, p_message_bytes, buffer_size);
      if (RAFT_FAILURE(status)) {
        return status;
      }
      status = raft_recv_request_vote(p_state, &args);
      break;
    }
    case MSG_TYPE_REQUEST_VOTE_RESPONSE:
    {
      raft_request_vote_response_args_t args;
      status = raft_read_request_vote_response_args(&args,
                                                    p_message_bytes,
                                                    buffer_size);
      if (RAFT_FAILURE(status)) {
        return status;
      }
      status = raft_recv_request_vote_response(p_state, &args);
      break;
    }
    default:
    {
      RAFT_ASSERT(RAFT_FALSE);
      break;
    }
  }
  return status;
}

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
             " term from another leader. Sender id: %u.",
             p_args->leader_id);
    return RAFT_STATUS_INVALID_ARGS;
  }

  on_leader_ping(p_state);

  raft_log_entry_t const* p_prev_entry;
  p_prev_entry = raft_log_entry(p_log, p_args->prev_log_index);
  if (p_prev_entry == NULL || p_prev_entry->term != p_args->prev_log_term) {
    // TODO: Delete unexpected entries. Enqueue response.
    return RAFT_STATUS_OK;
  }

  raft_log_append(p_state->p.p_log, p_args->p_log_entries, p_args->num_entries);

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
  RAFT_LOG(p_state, "Received vote from nodeid: %u, term: %u",
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

    RAFT_LOG(p_state, "Voting for nodeid: %u, term: %u.",
             p_args->candidate_id, p_args->term);
    p_state->p.voted_for = p_args->candidate_id;

    response.vote_granted = RAFT_TRUE;
    goto respond;
  }

respond:

  return send_request_vote_response(p_state, p_args->candidate_id, &response);
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

  if (raft_state_vote_count(p_state) > node_count / 2) {
    promote_to_leader(p_state);
  }

  return RAFT_STATUS_OK;
}

static raft_status_t promote_to_leader(raft_state_t* p_state) {
  RAFT_LOG(p_state, "Leader for term %u.",
           p_state->p.current_term);

  raft_state_set_type(p_state, RAFT_NODE_TYPE_LEADER);

  /* Send initial AppendEntries messages to establish leadership. */
  uint32_t const node_count = p_state->p_config->node_count;
  for (uint32_t i = 0; i < node_count; ++i) {
    /* TODO: Actually send the damned messages! */
  }

  return RAFT_STATUS_OK;
}

static void on_leader_ping(raft_state_t* p_state) {
  raft_config_t* p_config = p_state->p_config;

  p_state->v.ms_since_last_leader_ping = 0;
  uint32_t range = (p_config->election_timeout_max_ms -
                    p_config->election_timeout_min_ms);
  p_state->v.election_timeout_ms = ((rand() % range) +
                                    p_config->election_timeout_min_ms);
}


static raft_status_t send_request_vote_response(
    raft_state_t* p_state,
    raft_nodeid_t recipient_id,
    raft_request_vote_response_args_t* p_args) {
  raft_config_t const* p_config = p_state->p_config;

  raft_status_t status;
  if (p_config->cb.pf_request_vote_response_rpc) {
    status = p_config->cb.pf_request_vote_response_rpc(recipient_id, p_args);
  } else {
    raft_envelope_t envelope = { 0 };
    status = raft_write_request_vote_response_envelope(&envelope,
                                                       recipient_id, p_args);
    if (RAFT_SUCCESS(status)) {
      status = p_config->cb.pf_send_message(recipient_id,
                                            envelope.p_message,
                                            envelope.message_size);
    } else {
      raft_dealloc_envelope(&envelope);
    }
  }
  return status;
}
