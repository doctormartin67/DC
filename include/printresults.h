#ifndef PRINTRESULTS
#define PRINTRESULTS

/*
 * create an excel file called TestCase[tc] with all the values for a given
 * person in the data to test the results.
 */
void print_test_case(const CurrentMember *cm);
void print_results(const Database *db, const CurrentMember *cm);

#endif
