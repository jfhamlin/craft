

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void Test_priority_queue(CuTest*);
extern void Test_election(CuTest*);
extern void Test_election_after_leader_failures(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, Test_priority_queue);
    SUITE_ADD_TEST(suite, Test_election);
    SUITE_ADD_TEST(suite, Test_election_after_leader_failures);

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

