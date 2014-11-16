#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "raft.h"
#include "raft_config.h"
#include "raft_log.h"
#include "raft_util.h"

static raft_bool_t should_begin_election(raft_state_t* p_state);
static raft_status_t begin_election(raft_state_t* p_state);
static void* check_leader(void* arg);

raft_status_t raft_start(raft_state_t* p_state) {
  pthread_t thread;
  pthread_create(&thread, NULL, check_leader, p_state);

  return RAFT_STATUS_OK;
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
      p_config->cb.p_request_vote_rpc(p_config->p_nodeids[i], &args);
    }
  }

  return RAFT_STATUS_OK;
}

static raft_bool_t should_begin_election(raft_state_t* p_state) {
  if (p_state->type == RAFT_NODE_TYPE_LEADER)
    return RAFT_FALSE;

  raft_bool_t heard_from_leader = p_state->v.heard_from_leader;
  p_state->v.heard_from_leader = RAFT_FALSE;

  return !heard_from_leader;
}

static void* check_leader(void* arg) {
  raft_state_t* p_state = arg;

  for (;;) {
    raft_config_t const* p_config = p_state->p_config;

    if (p_state->type == RAFT_NODE_TYPE_LEADER) {
      raft_append_entries_args_t args = { 0 };
      args.term = p_state->p.current_term;

      for (uint32_t i = 0; i < p_config->node_count - 1; ++i) {
        if (p_config->p_nodeids[i] != p_state->p.self) {
          p_config->cb.p_append_entries_rpc(p_config->p_nodeids[i], &args);
        }
      }
      usleep(4500 * 1000); // How long between keepalives?
      continue;
    }

    uint32_t range = (p_config->election_timeout_max_ms -
                      p_config->election_timeout_min_ms);
    uint32_t sleep_ms = (rand() % range) + p_config->election_timeout_min_ms;
    usleep(1000 * sleep_ms);

    if (should_begin_election(p_state)) {
      begin_election(p_state);
    }
  }

  return NULL;
}
