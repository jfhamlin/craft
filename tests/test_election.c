#include <stdio.h>

#include "CuTest.h"

#define STB_DEFINE
#include "stb.h"

#define NODE_COUNT 5
#include "test_helpers.h"

void Test_priority_queue(CuTest* tc) {
  int const num_count = 3;
  int* p_pq = NULL;
  for (int i = 0; i < num_count; ++i) {
    stb_pq_push(p_pq, i, cmp_int);
  }
  CuAssertIntEquals(tc, stb_pq_len(p_pq), num_count);

  stb_pq_pop(p_pq, cmp_int);

  CuAssertIntEquals(tc, stb_pq_len(p_pq), num_count-1);

  int v = 0;
  stb_pq_push(p_pq, v, cmp_int);

  CuAssertIntEquals(tc, stb_pq_len(p_pq), num_count);

  for (int i = 0; i < num_count; ++i) {
    CuAssertIntEquals(tc, stb_pq_min(p_pq), i);
    stb_pq_pop(p_pq, cmp_int);
  }
}

void Test_election(CuTest* tc) {
  start_nodes();

  CuAssertPtrEquals(tc, first_leader(), NULL);

  process_events(10);

  CuAssertIntEquals(tc, leader_count(), 1);

  STOP_NODES();
}

void Test_election_after_leader_failures(CuTest* tc) {
  start_nodes();

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  CuAssertIntEquals(tc, active_node_count(), event_count());

  raft_stop(first_leader());
  CuAssertIntEquals(tc, 0, leader_count());

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  raft_stop(first_leader());

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  CuAssertIntEquals(tc, active_node_count(), event_count());

  raft_state_t* p_leader = first_leader();
  raft_stop(p_leader);

  process_events(10);
  CuAssertIntEquals(tc, 0, leader_count());

  CuAssertIntEquals(tc, active_node_count(), event_count());

  raft_start(p_leader);

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  STOP_NODES();
}
