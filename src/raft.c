#include <stdlib.h>
#include <string.h>

#include "raft_util.h"
#include "raft_state.h"
#include "raft_config.h"
#include "raft_log.h"

static raft_bool_t should_begin_election(raft_state_t* p_state);
static raft_status_t begin_election(raft_state_t* p_state);

raft_status_t raft_alloc(raft_state_t** pp_state, raft_config_t* p_config) {
  *pp_state = NULL;

  raft_state_t* p_state = calloc(1, sizeof(raft_state_t));
  if (p_state == NULL) {
    return RAFT_STATUS_OUT_OF_MEMORY;
  }

  p_state->type = RAFT_NODE_TYPE_FOLLOWER;
  p_state->p.self = p_config->selfid;

  /**
   * Create the log.
   */
  raft_log_t* p_log = p_state->p.p_log = raft_log_create();
  if (p_log == NULL) {
    free(p_state);
    return RAFT_STATUS_OUT_OF_MEMORY;
  }

  p_state->p_config = p_config;

  /**
   * Validate election timeout settings and set initial timeout.
   */
  if (p_config->election_timeout_max_ms <= p_config->election_timeout_min_ms) {
    raft_log_free(p_log);
    free(p_state);
    RAFT_LOG(p_state,
             "Invalid election_timeout settings: election_timeout_max_ms (%u) <= election_timeout_min_ms (%u)",
             p_config->election_timeout_max_ms,
             p_config->election_timeout_min_ms);
    return RAFT_STATUS_INVALID_ARGS;
  }

  uint32_t range = (p_config->election_timeout_max_ms -
                    p_config->election_timeout_min_ms);
  p_state->v.election_timeout_ms = ((rand() % range) +
                                    p_config->election_timeout_min_ms);

  /**
   * Allocate and initialize the ballot.
   */
  uint32_t const ballot_size = p_config->node_count * sizeof(raft_bool_t);
  p_state->l.p_ballot = calloc(1, ballot_size);
  if (p_state->l.p_ballot == NULL) {
    raft_log_free(p_log);
    free(p_state);
    return RAFT_STATUS_OUT_OF_MEMORY;
  }

  *pp_state = p_state;
  return RAFT_STATUS_OK;
}

void raft_free(raft_state_t* p_state) {
  // TODO: Make sure everything is actually freed...
  free(p_state->l.p_ballot);
  raft_log_free(p_state->p.p_log);
  free(p_state);
}

static raft_status_t begin_election(raft_state_t* p_state) {
  RAFT_LOG(p_state, "Beginning election!");

  raft_term_t election_term;

  {
    raft_state_set_type(p_state, RAFT_NODE_TYPE_CANDIDATE);
    p_state->p.voted_for = p_state->p.self;
    election_term = ++p_state->p.current_term;
  }

  /* Vote for self. */
  {
    raft_bool_t* p_ballot = p_state->l.p_ballot;
    memset(p_ballot, 0, p_state->p_config->node_count * sizeof(*p_ballot));
    p_ballot[p_state->p.self - 1] = RAFT_TRUE;
  }

  raft_request_vote_args_t args = { 0 };
  args.term = election_term;
  args.candidate_id = p_state->p.self;

  raft_log_t* p_log = p_state->p.p_log;
  args.last_log_index = raft_log_length(p_log);
  args.last_log_term = (args.last_log_index > 0 ?
                        raft_log_entry(p_log, -1)->term :
                        0);

  raft_config_t* p_config = p_state->p_config;
  for (uint32_t i = 0; i < p_config->node_count - 1; ++i) {
    if (p_config->p_nodeids[i] != p_state->p.self) {
      p_config->cb.pf_request_vote_rpc(p_config->p_nodeids[i], &args);
    }
  }

  return RAFT_STATUS_OK;
}

static raft_bool_t should_begin_election(raft_state_t* p_state) {
  if (p_state->type == RAFT_NODE_TYPE_LEADER)
    return RAFT_FALSE;


  return p_state->v.ms_since_last_leader_ping >= p_state->v.election_timeout_ms;
}

raft_status_t raft_tick(uint32_t* p_reschedule_ms,
                        raft_state_t* p_state,
                        uint32_t elapsed_ms) {
  raft_status_t status = RAFT_STATUS_OK;

  p_state->v.ms_since_last_leader_ping += elapsed_ms;
  RAFT_LOG(p_state,
           "TICK: ms since last leader ping: %u, election_timeout: %u",
           p_state->v.ms_since_last_leader_ping,
           p_state->v.election_timeout_ms);

  raft_config_t const* p_config = p_state->p_config;

  if (p_state->type == RAFT_NODE_TYPE_LEADER) {
    raft_append_entries_args_t args = { 0 };
    args.term = p_state->p.current_term;

    for (uint32_t i = 0; i < p_config->node_count - 1; ++i) {
      if (p_config->p_nodeids[i] != p_state->p.self) {
        p_config->cb.pf_append_entries_rpc(p_config->p_nodeids[i], &args);
      }
    }

    *p_reschedule_ms = p_config->leader_ping_interval_ms;
  } else {
    if (should_begin_election(p_state)) {
      if (RAFT_FAILURE(status = begin_election(p_state))) {
        return status;
      }
    }

    *p_reschedule_ms = (p_state->v.election_timeout_ms -
                        p_state->v.ms_since_last_leader_ping);
  }

  return RAFT_STATUS_OK;
}
