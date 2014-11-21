#ifndef __RAFT_CALLBACKS_H__
#define __RAFT_CALLBACKS_H__

#include "raft_rpc.h"

typedef raft_status_t raft_append_entries_rpc_t(
    raft_nodeid_t,
    raft_append_entries_args_t*
);

typedef raft_status_t raft_append_entries_response_rpc_t(
    raft_nodeid_t,
    raft_append_entries_response_args_t*
);

typedef raft_status_t raft_request_vote_rpc_t(
    raft_nodeid_t,
    raft_request_vote_args_t*
);

typedef raft_status_t raft_request_vote_response_rpc_t(
    raft_nodeid_t,
    raft_request_vote_response_args_t*
);

typedef raft_status_t raft_schedule_event_t(
    raft_state_t* p_state,
    uint32_t ms_from_now,
    raft_event_type_t event_type
);

typedef struct {
  raft_append_entries_rpc_t*          p_append_entries_rpc;
  raft_append_entries_response_rpc_t* p_append_entries_response_rpc;

  raft_request_vote_rpc_t*          p_request_vote_rpc;
  raft_request_vote_response_rpc_t* p_request_vote_response_rpc;

  raft_schedule_event_t* p_schedule_event;
} raft_callbacks_t;

#endif
