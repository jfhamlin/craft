#ifndef __RAFT_RPC_H__
#define __RAFT_RPC_H__

#include "raft_types.h"

typedef struct raft_state raft_state_t;
typedef struct raft_log_entry raft_log_entry_t;

/**
 *
 */
raft_status_t raft_recv_message(raft_state_t* p_state,
                                void* p_message_bytes,
                                uint32_t buffer_size);

typedef struct {
  raft_term_t        term;
  raft_nodeid_t      leader_id;
  raft_index_t       prev_log_index;
  raft_term_t        prev_log_term;
  raft_log_entry_t*  p_log_entries;
  uint32_t           num_entries;
  uint32_t           leader_commit;
} raft_append_entries_args_t;

raft_status_t
raft_recv_append_entries(raft_state_t* p_state,
                         raft_append_entries_args_t* p_args);

typedef struct {
  raft_nodeid_t follower_id;

  raft_term_t term;
  raft_bool_t success;

  raft_index_t acknowledged_log_index;
  raft_index_t acknowledged_log_term;
} raft_append_entries_response_args_t;

raft_status_t
raft_recv_append_entries_response(raft_state_t* p_state,
                                  raft_append_entries_response_args_t* p_args);


typedef struct {
  raft_term_t   term;
  raft_nodeid_t candidate_id;
  raft_index_t  last_log_index;
  raft_term_t   last_log_term;
} raft_request_vote_args_t;

raft_status_t
raft_recv_request_vote(raft_state_t* p_state,
                       raft_request_vote_args_t* p_args);

typedef struct {
  raft_nodeid_t follower_id;

  raft_term_t   term;
  raft_bool_t   vote_granted;
} raft_request_vote_response_args_t;

raft_status_t
raft_recv_request_vote_response(raft_state_t* p_state,
                                raft_request_vote_response_args_t* p_args);

#endif
