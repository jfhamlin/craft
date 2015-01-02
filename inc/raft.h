#ifndef __RAFT_H__
#define __RAFT_H__

#include "raft_config.h"
#include "raft_rpc.h"

typedef struct raft_state raft_state_t;

/**
 *
 */
raft_status_t raft_alloc(raft_state_t** pp_state, raft_config_t* p_config);

/**
 *
 */
void raft_free(raft_state_t* p_state);

/**
 *
 */
raft_status_t raft_tick(uint32_t* p_reschedule_ms,
                        raft_state_t* p_state,
                        uint32_t elapsed_ms);

/**
 *
 */
//raft_status_t raft_append(raft_user_entry_t* p_entry);

#endif
