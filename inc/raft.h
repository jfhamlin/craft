#ifndef __RAFT_H__
#define __RAFT_H__

#include "raft_types.h"

typedef struct raft_state raft_state_t;
typedef struct raft_config raft_config_t;

/**
 *
 */
raft_status_t raft_init(raft_state_t** pp_state, raft_config_t* p_config);

/**
 *
 */
void raft_deinit(raft_state_t* p_state);

/**
 *
 */
raft_status_t raft_start(raft_state_t* p_state);

/**
 *
 */
raft_status_t raft_stop(raft_state_t* p_state);

/**
 *
 */
raft_status_t raft_receive_event(raft_state_t* p_state, raft_event_type_t type);

/**
 *
 */
//raft_status_t raft_append(raft_user_entry_t* p_entry);

#endif
