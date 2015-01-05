#include <stdio.h>

#include "CuTest.h"

#define NODE_COUNT 5
#include "test_helpers.h"

void Test_election(CuTest* tc) {
  start_nodes();

  CuAssertPtrEquals(tc, get_node(first_leader()), NULL);

  process_events(10);

  CuAssertIntEquals(tc, 1, leader_count());

  stop_nodes();
}

void Test_election_after_leader_failures(CuTest* tc) {
  start_nodes();

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  stop_node(first_leader());
  CuAssertIntEquals(tc, 0, leader_count());

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  stop_node(first_leader());

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  stop_node(first_leader());

  process_events(10);
  CuAssertIntEquals(tc, 0, leader_count());

  stop_nodes();
}
