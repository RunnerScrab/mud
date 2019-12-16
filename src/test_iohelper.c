#include "charvector.h"
#include "iohelper.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>

int test_pipe[2] = {0};
void openpipe(void)
{

}

void closepipe(void)
{

}



START_TEST(iohelper_test_read)
{
	printf("In read test.\n");
	cv_t readbuf;
	cv_init(&readbuf, 64);
	size_t readbytes = read_to_cv(test_pipe[0], &readbuf, 256);

	printf("Read %lu bytes.\n", readbytes);
	ck_assert(readbytes == 256);
	size_t idx = 0, sum = 0;
	for(; idx < readbytes; ++idx)
	{
		unsigned char ch = cv_at(&readbuf, idx);
		sum += ch;
	}
	ck_assert(sum == 32640);
	cv_destroy(&readbuf);
}
END_TEST

START_TEST(iohelper_test_write)
{
	cv_t sendbuf;
	cv_init(&sendbuf, 512);
	size_t idx = 0;
	for(; idx < 512; ++idx)
	{
		cv_push(&sendbuf, (unsigned char) idx);
	}
	ck_assert(write_from_cv(test_pipe[1], &sendbuf) == 512);

	printf("Write test complete\n");
	cv_destroy(&sendbuf);
}
END_TEST

START_TEST(memoryleak_check)
{
	ck_assert(toutstanding_allocs() == 0);
}
END_TEST

Suite* cv_test_suite(void)
{
	Suite* s = suite_create("iohelper");
	TCase* testcases = tcase_create("Core");
	tcase_add_checked_fixture(testcases, openpipe, closepipe);
	tcase_add_test(testcases, iohelper_test_write);
	tcase_add_test(testcases, iohelper_test_read);
	tcase_add_test(testcases, memoryleak_check);


	suite_add_tcase(s, testcases);

	return s;
}


int main(void)
{
	pipe(test_pipe);
	size_t tests_failed = 0;
	Suite* s = cv_test_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	tests_failed = srunner_ntests_failed(sr);

	srunner_free(sr);
	close(test_pipe[0]);
	close(test_pipe[1]);
	talloc_subsys_release();
	return (tests_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
