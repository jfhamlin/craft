

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void Test_priority_queue(CuTest*);
extern void Test_election(CuTest*);
extern void Test_election_after_leader_failures(CuTest*);
extern void Test_raft_recv_request_vote_With_new_term(CuTest*);
extern void Test_raft_recv_request_vote_With_old_term(CuTest*);
extern void Test_raft_recv_request_vote_With_same_term_and_voted_for(CuTest*);
extern void Test_raft_recv_request_vote_With_new_term_but_old_log(CuTest*);
extern void Test_raft_recv_request_vote_While_leader_with_current_term(CuTest*);
extern void Test_raft_recv_request_vote_While_leader_with_new_term_and_old_log(CuTest*);
extern void Test_raft_recv_request_vote_While_leader_with_new_term_and_same_log(CuTest*);
extern void Test_raft_recv_request_vote_While_leader_with_new_term_and_new_log(CuTest*);
extern void Test_raft_recv_request_vote_response(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, Test_priority_queue);
    SUITE_ADD_TEST(suite, Test_election);
    SUITE_ADD_TEST(suite, Test_election_after_leader_failures);
    SUITE_ADD_TEST(suite, Test_raft_recv_request_vote_With_new_term);
    SUITE_ADD_TEST(suite, Test_raft_recv_request_vote_With_old_term);
    SUITE_ADD_TEST(suite, Test_raft_recv_request_vote_With_same_term_and_voted_for);
    SUITE_ADD_TEST(suite, Test_raft_recv_request_vote_With_new_term_but_old_log);
    SUITE_ADD_TEST(suite, Test_raft_recv_request_vote_While_leader_with_current_term);
    SUITE_ADD_TEST(suite, Test_raft_recv_request_vote_While_leader_with_new_term_and_old_log);
    SUITE_ADD_TEST(suite, Test_raft_recv_request_vote_While_leader_with_new_term_and_same_log);
    SUITE_ADD_TEST(suite, Test_raft_recv_request_vote_While_leader_with_new_term_and_new_log);
    SUITE_ADD_TEST(suite, Test_raft_recv_request_vote_response);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\\n", output->buffer);
}

int main(int argc, char **argv)
{
    RunAllTests();
    return 0;
}

