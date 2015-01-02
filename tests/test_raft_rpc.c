#include "CuTest.h"

#include "raft_rpc.h"
#include "raft_log.h"

#define NODE_COUNT 5
#include "test_helpers.h"

/*******************************************************************************
 *******************************************************************************
 *************************** Receive RequestVote RPC ***************************
 *******************************************************************************
 ******************************************************************************/

static raft_nodeid_t s_recvd_vote_response_target;
static raft_request_vote_response_args_t s_recvd_vote_response_args;
static raft_status_t save_request_vote_response_rpc(
    raft_nodeid_t id, raft_request_vote_response_args_t* p_args) {
  s_recvd_vote_response_target = id;
  s_recvd_vote_response_args = *p_args;
  return RAFT_STATUS_OK;
}

#define TEST_REQUEST_VOTE_SETUP(node_type, term)                        \
  raft_state_t* p_state = make_raft_node(1);                            \
  p_state->type = (node_type);                                          \
  p_state->p.current_term = (term);                                     \
  if ((node_type) == RAFT_NODE_TYPE_LEADER)                             \
    p_state->p.voted_for = p_state->p.self;                             \
  p_state->p_config->cb.pf_request_vote_response_rpc = save_request_vote_response_rpc

void Test_raft_recv_request_vote_With_new_term(CuTest* tc) {
  TEST_REQUEST_VOTE_SETUP(RAFT_NODE_TYPE_FOLLOWER, 0);

  /* With new term. */
  raft_request_vote_args_t args = {
    .term = 1,
    .candidate_id = 2,
    .last_log_index = 0,
    .last_log_term = 0
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_FOLLOWER, p_state->type);
  CuAssertIntEquals(tc, 1, p_state->p.current_term);
  CuAssertIntEquals(tc, 2, s_recvd_vote_response_target);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.follower_id);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.term);
  CuAssertTrue(tc, s_recvd_vote_response_args.vote_granted);

  /* With repeated message */
  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_FOLLOWER, p_state->type);
  CuAssertIntEquals(tc, 1, p_state->p.current_term);
  CuAssertIntEquals(tc, 2, s_recvd_vote_response_target);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.follower_id);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.term);
  CuAssertTrue(tc, s_recvd_vote_response_args.vote_granted);

  STOP_NODES();
}

void Test_raft_recv_request_vote_With_old_term(CuTest* tc) {
  TEST_REQUEST_VOTE_SETUP(RAFT_NODE_TYPE_FOLLOWER, 1);

  /* With old term. */
  raft_request_vote_args_t args = {
    .term = 0,
    .candidate_id = 3,
    .last_log_index = 0,
    .last_log_term = 0
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_FOLLOWER, p_state->type);
  CuAssertIntEquals(tc, 1, p_state->p.current_term);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_target);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.follower_id);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.term);
  CuAssertTrue(tc, !s_recvd_vote_response_args.vote_granted);

  STOP_NODES();
}

void Test_raft_recv_request_vote_With_same_term_and_voted_for(CuTest* tc) {
  TEST_REQUEST_VOTE_SETUP(RAFT_NODE_TYPE_FOLLOWER, 1);
  p_state->p.voted_for = 2;

  /* With same term and non-null candidate_id. */
  raft_request_vote_args_t args = {
    .term = 1,
    .candidate_id = 3,
    .last_log_index = 0,
    .last_log_term = 0
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_FOLLOWER, p_state->type);
  CuAssertIntEquals(tc, 1, p_state->p.current_term);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_target);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.follower_id);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.term);
  CuAssertTrue(tc, !s_recvd_vote_response_args.vote_granted);

  STOP_NODES();
}

void Test_raft_recv_request_vote_With_new_term_but_old_log(CuTest* tc) {
  TEST_REQUEST_VOTE_SETUP(RAFT_NODE_TYPE_FOLLOWER, 1);
  p_state->p.voted_for = 2;

  /* With new term with out-of-date log. */
  raft_log_entry_t* p_entry = (raft_log_entry_t*)raft_log_entry(p_state->p.p_log, 0);
  p_entry->term = 1;

  raft_request_vote_args_t args = {
    .term = 2,
    .candidate_id = 3,
    .last_log_index = 0,
    .last_log_term = 0
  };

  /* Should update current_term. */
  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_FOLLOWER, p_state->type);
  CuAssertIntEquals(tc, 0, p_state->p.voted_for);
  CuAssertIntEquals(tc, 2, p_state->p.current_term);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_target);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.follower_id);
  CuAssertIntEquals(tc, 2, s_recvd_vote_response_args.term);
  CuAssertTrue(tc, !s_recvd_vote_response_args.vote_granted);

  STOP_NODES();
}

void Test_raft_recv_request_vote_While_leader_with_current_term(CuTest* tc) {
  TEST_REQUEST_VOTE_SETUP(RAFT_NODE_TYPE_LEADER, 2);

  raft_request_vote_args_t args = {
    .term = 2,
    .candidate_id = 3,
    .last_log_index = 0,
    .last_log_term = 1
  };

  /* Nothing should change */
  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_LEADER, p_state->type);
  CuAssertIntEquals(tc, 2, p_state->p.current_term);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_target);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.follower_id);
  CuAssertIntEquals(tc, 2, s_recvd_vote_response_args.term);
  CuAssertTrue(tc, !s_recvd_vote_response_args.vote_granted);

  STOP_NODES();
}

void Test_raft_recv_request_vote_While_leader_with_new_term_and_old_log(CuTest* tc) {
  TEST_REQUEST_VOTE_SETUP(RAFT_NODE_TYPE_LEADER, 2);

  raft_log_entry_t* p_entry = (raft_log_entry_t*)raft_log_entry(p_state->p.p_log, 0);
  p_entry->term = 1;

  raft_request_vote_args_t args = {
    .term = 3,
    .candidate_id = 3,
    .last_log_index = 0,
    .last_log_term = 0
  };

  /* Should become a follower but deny vote. */
  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_FOLLOWER, p_state->type);
  CuAssertIntEquals(tc, 3, p_state->p.current_term);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_target);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.follower_id);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_args.term);
  CuAssertTrue(tc, !s_recvd_vote_response_args.vote_granted);

  STOP_NODES();
}

void Test_raft_recv_request_vote_While_leader_with_new_term_and_same_log(CuTest* tc) {
  TEST_REQUEST_VOTE_SETUP(RAFT_NODE_TYPE_LEADER, 2);

  raft_log_entry_t* p_entry = (raft_log_entry_t*)raft_log_entry(p_state->p.p_log, 0);
  p_entry->term = 1;

  raft_request_vote_args_t args = {
    .term = 3,
    .candidate_id = 3,
    .last_log_index = 0,
    .last_log_term = 1
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_FOLLOWER, p_state->type);
  CuAssertIntEquals(tc, 3, p_state->p.voted_for);
  CuAssertIntEquals(tc, 3, p_state->p.current_term);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_target);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.follower_id);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_args.term);
  CuAssertTrue(tc, s_recvd_vote_response_args.vote_granted);

  STOP_NODES();
}

void Test_raft_recv_request_vote_While_leader_with_new_term_and_new_log(CuTest* tc) {
  TEST_REQUEST_VOTE_SETUP(RAFT_NODE_TYPE_LEADER, 2);

  raft_request_vote_args_t args = {
    .term = 3,
    .candidate_id = 3,
    .last_log_index = 10,
    .last_log_term = 0
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_FOLLOWER, p_state->type);
  CuAssertIntEquals(tc, 3, p_state->p.voted_for);
  CuAssertIntEquals(tc, 3, p_state->p.current_term);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_target);
  CuAssertIntEquals(tc, 1, s_recvd_vote_response_args.follower_id);
  CuAssertIntEquals(tc, 3, s_recvd_vote_response_args.term);
  CuAssertTrue(tc, s_recvd_vote_response_args.vote_granted);

  STOP_NODES();
}

/*******************************************************************************
 *******************************************************************************
 ********************** Receive RequestVoteResponse RPC ************************
 *******************************************************************************
 ******************************************************************************/

#define TEST_REQUEST_VOTE_RESPONSE_SETUP(node_type, term)               \
  raft_state_t* p_state = make_raft_node(1);                            \
  p_state->type = (node_type);                                          \
  p_state->p.current_term = (term);                                     \
  if ((node_type) == RAFT_NODE_TYPE_LEADER || (node_type) == RAFT_NODE_TYPE_CANDIDATE) { \
    p_state->p.voted_for = p_state->p.self;                             \
    p_state->l.p_ballot[0] = RAFT_TRUE;                                 \
  }                                                                     \
  (void)0

void Test_raft_recv_request_vote_response_with_granted_vote(CuTest* tc) {
  TEST_REQUEST_VOTE_RESPONSE_SETUP(RAFT_NODE_TYPE_CANDIDATE, 1);

  raft_request_vote_response_args_t args = {
    .follower_id = 2,
    .term = 1,
    .vote_granted = RAFT_TRUE
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote_response(p_state, &args));
  CuAssertIntEquals(tc, 2, raft_state_vote_count(p_state));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_CANDIDATE, p_state->type);

  STOP_NODES();
}

void Test_raft_recv_request_vote_response_with_denied_vote(CuTest* tc) {
  TEST_REQUEST_VOTE_RESPONSE_SETUP(RAFT_NODE_TYPE_CANDIDATE, 1);

  raft_request_vote_response_args_t args = {
    .follower_id = 2,
    .term = 1,
    .vote_granted = RAFT_FALSE
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK, raft_recv_request_vote_response(p_state, &args));
  CuAssertIntEquals(tc, 1, raft_state_vote_count(p_state));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_CANDIDATE, p_state->type);

  STOP_NODES();
}

void Test_raft_recv_request_vote_response_with_future_term(CuTest* tc) {
  TEST_REQUEST_VOTE_RESPONSE_SETUP(RAFT_NODE_TYPE_CANDIDATE, 1);

  raft_request_vote_response_args_t args = {
    .follower_id = 2,
    .term = 2,
    .vote_granted = RAFT_TRUE
  };

  CuAssertIntEquals(tc, RAFT_STATUS_INVALID_TERM,
                    raft_recv_request_vote_response(p_state, &args));
  CuAssertIntEquals(tc, 1, raft_state_vote_count(p_state));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_CANDIDATE, p_state->type);

  STOP_NODES();
}

void Test_raft_recv_request_vote_response_with_past_term(CuTest* tc) {
  TEST_REQUEST_VOTE_RESPONSE_SETUP(RAFT_NODE_TYPE_CANDIDATE, 2);

  raft_request_vote_response_args_t args = {
    .follower_id = 2,
    .term = 1,
    .vote_granted = RAFT_TRUE
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_recv_request_vote_response(p_state, &args));
  CuAssertIntEquals(tc, 1, raft_state_vote_count(p_state));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_CANDIDATE, p_state->type);

  STOP_NODES();
}

void Test_raft_recv_request_vote_response_with_invalid_state(CuTest* tc) {
  TEST_REQUEST_VOTE_RESPONSE_SETUP(RAFT_NODE_TYPE_FOLLOWER, 1);

  raft_request_vote_response_args_t args = {
    .follower_id = 2,
    .term = 1,
    .vote_granted = RAFT_TRUE
  };

  CuAssertIntEquals(tc, RAFT_STATUS_INVALID_ARGS,
                    raft_recv_request_vote_response(p_state, &args));
  CuAssertIntEquals(tc, 0, raft_state_vote_count(p_state));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_FOLLOWER, p_state->type);

  STOP_NODES();
}

void Test_raft_recv_request_vote_response_with_unknown_id(CuTest* tc) {
  TEST_REQUEST_VOTE_RESPONSE_SETUP(RAFT_NODE_TYPE_CANDIDATE, 1);

  raft_request_vote_response_args_t args = {
    .follower_id = 9999,
    .term = 1,
    .vote_granted = RAFT_TRUE
  };

  CuAssertIntEquals(tc, RAFT_STATUS_INVALID_ARGS,
                    raft_recv_request_vote_response(p_state, &args));
  CuAssertIntEquals(tc, 1, raft_state_vote_count(p_state));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_CANDIDATE, p_state->type);

  STOP_NODES();
}

void Test_raft_recv_request_vote_response_as_leader(CuTest* tc) {
  TEST_REQUEST_VOTE_RESPONSE_SETUP(RAFT_NODE_TYPE_LEADER, 1);

  raft_request_vote_response_args_t args = {
    .follower_id = 2,
    .term = 1,
    .vote_granted = RAFT_TRUE
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_recv_request_vote_response(p_state, &args));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_LEADER, p_state->type);

  STOP_NODES();
}

void Test_raft_recv_request_vote_leader_promotion(CuTest* tc) {
  TEST_REQUEST_VOTE_RESPONSE_SETUP(RAFT_NODE_TYPE_CANDIDATE, 1);
  for (uint32_t i = 3; i <= (p_state->p_config->node_count >> 1) + 1; ++i) {
    p_state->l.p_ballot[i-1] = RAFT_TRUE;
  }

  CuAssertIntEquals(tc, (p_state->p_config->node_count >> 1),
                    raft_state_vote_count(p_state));

  raft_request_vote_response_args_t args = {
    .follower_id = 2,
    .term = 1,
    .vote_granted = RAFT_TRUE
  };

  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_recv_request_vote_response(p_state, &args));
  CuAssertIntEquals(tc, (p_state->p_config->node_count >> 1) + 1,
                    raft_state_vote_count(p_state));
  CuAssertIntEquals(tc, RAFT_NODE_TYPE_LEADER, p_state->type);

  CuFail(tc, "PENDING: Check that leader assertion messages are enqueued!");

  STOP_NODES();
}
