#include "CuTest.h"

#include "raft_wire.h"

#define NODE_COUNT 5
#include "test_helpers.h"

/*******************************************************************************
 *******************************************************************************
 *************************** RequestVote Wire Format ***************************
 *******************************************************************************
 ******************************************************************************/

static uint8_t expected_request_vote_message[] = {
  0, 0, 1, MSG_TYPE_REQUEST_VOTE,
  0, 0, 0, 24,
  0x55, 0x44, 0x33, 0x22,
  0x99, 0x88, 0x77, 0x66,
  0x11, 0x22, 0x33, 0x44,
  0x00, 0x11, 0x00, 0x33,
};

void Test_raft_write_request_vote_envelope(CuTest* tc) {
  raft_request_vote_args_t args = {
    .term = 0x55443322,
    .candidate_id = 0x99887766,
    .last_log_index = 0x11223344,
    .last_log_term = 0x00110033
  };

  raft_envelope_t env = { 0 };
  raft_write_request_vote_envelope(&env, 1, &args);

  CuAssertIntEquals(tc, 1, env.recipient_id);
  CuAssertIntEquals(tc, 24, env.message_size);
  CuAssertIntEquals(tc, 0x100, env.buffer_capacity);

  uint32_t const arr_size = ARRAY_ELEMENT_COUNT(expected_request_vote_message);
  for (uint32_t i = 0; i < arr_size; ++i) {
    CuAssertIntEquals(tc, expected_request_vote_message[i], env.p_message[i]);
  }
}

void Test_raft_read_request_vote_message(CuTest* tc) {
  raft_request_vote_args_t args = { 0 };

  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_read_request_vote_args(
                        &args,
                        expected_request_vote_message,
                        sizeof(expected_request_vote_message)));

  CuAssertIntEquals(tc, 0x55443322, args.term);
  CuAssertIntEquals(tc, 0x99887766, args.candidate_id);
  CuAssertIntEquals(tc, 0x11223344, args.last_log_index);
  CuAssertIntEquals(tc, 0x00110033, args.last_log_term);
}

/*******************************************************************************
 *******************************************************************************
 ********************** RequestVoteResponse Wire Format ************************
 *******************************************************************************
 ******************************************************************************/

static uint8_t expected_request_vote_response_message[] = {
  0, 0, 1, MSG_TYPE_REQUEST_VOTE_RESPONSE,
  0, 0, 0, 20,
  0x55, 0x44, 0x33, 0x22,
  0x99, 0x88, 0x77, 0x66,
  0, 0, 0, 1,
};

void Test_raft_write_request_vote_response_envelope(CuTest* tc) {
  raft_request_vote_response_args_t args = {
    .follower_id = 0x55443322,
    .term = 0x99887766,
    .vote_granted = RAFT_TRUE
  };

  raft_envelope_t env = { 0 };
  raft_write_request_vote_response_envelope(&env, 1, &args);

  CuAssertIntEquals(tc, 1, env.recipient_id);
  CuAssertIntEquals(tc, 20, env.message_size);
  CuAssertIntEquals(tc, 0x100, env.buffer_capacity);

  uint32_t const arr_size = ARRAY_ELEMENT_COUNT(expected_request_vote_response_message);
  for (uint32_t i = 0; i < arr_size; ++i) {
    CuAssertIntEquals(tc,
                      expected_request_vote_response_message[i],
                      env.p_message[i]);
  }
}

void Test_raft_read_request_vote_response_message(CuTest* tc) {
  raft_request_vote_response_args_t args = { 0 };

  CuAssertIntEquals(tc, RAFT_STATUS_OK,
                    raft_read_request_vote_response_args(
                        &args,
                        expected_request_vote_response_message,
                        sizeof(expected_request_vote_response_message)));

  CuAssertIntEquals(tc, 0x55443322, args.follower_id);
  CuAssertIntEquals(tc, 0x99887766, args.term);
  CuAssertIntEquals(tc, 0x1, args.vote_granted);
}
