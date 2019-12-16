#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include "charvector.h"

START_TEST(cv_append_and_push)
{
	size_t startallocs = tget_allocs() + tget_reallocs();
	cv_t str;
	cv_init(&str, 16);
	size_t idx = 0, z = 0;
	for(; idx < 10; ++idx)
		cv_push(&str, idx);

	char nums[] = {10, 11, 12, 13, 14};
	cv_append(&str, nums, 5);
	int sum = 0;
	for(idx = 0, z = cv_len(&str); idx < z; ++idx)
	{
		sum += cv_at(&str, idx);
	}
//	printf("%lu (re)allocs.\n", (tget_allocs() + tget_reallocs()) - startallocs);
	ck_assert((tget_allocs() + tget_reallocs()) - startallocs < 5);
	ck_assert_int_eq(105, sum);
	cv_destroy(&str);
}
END_TEST

START_TEST(cv_as_string)
{
	cv_t str;
	cv_init(&str, 1);
	cv_sprintf(&str, "Meow %d u", 2);
	ck_assert(strstr(str.data, "Meow 2 u"));
	ck_assert_int_eq(0, cv_at(&str, cv_len(&str)));
	cv_destroy(&str);
}
END_TEST

START_TEST(memoryleak_check)
{
	ck_assert(toutstanding_allocs() == 0);
}
END_TEST

Suite* cv_test_suite(void)
{
	Suite* s = suite_create("cv");
	TCase* testcases = tcase_create("Core");

	tcase_add_test(testcases, cv_append_and_push);
	tcase_add_test(testcases, cv_as_string);
	tcase_add_test(testcases, memoryleak_check);
	suite_add_tcase(s, testcases);

	return s;
}


int main(void)
{
	size_t tests_failed = 0;
	Suite* s = cv_test_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	tests_failed = srunner_ntests_failed(sr);

	srunner_free(sr);

	talloc_subsys_release();
	return (tests_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
