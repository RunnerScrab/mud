#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <check.h>

#include "../heap.h"
#include "../talloc.h"

int randint(int min, int max)
{
	return rand() % max + min;
}

START_TEST(heap_create)
{
	struct Heap testheap;
	Heap_Create(&testheap, 10);

	size_t idx = 0;
	for(; idx < 10; ++idx)
	{
		Heap_MinInsert(&testheap, idx, 0);
	}

	ck_assert_int_eq(Heap_GetSize(&testheap), 10);
	Heap_Destroy(&testheap);
}
END_TEST

START_TEST(heap_insert)
{
	struct Heap testheap;
	Heap_Create(&testheap, 10);
	size_t idx = 0, testiterations = 10000;
	for(; idx < testiterations; ++idx)
	{
		unsigned long long keytest = (unsigned long long) randint(1, testiterations << 1);
		Heap_MinInsert(&testheap, keytest, (void*) keytest);
	}
	unsigned long long lastval = 0;
	for(idx = 0; idx < testiterations; ++idx)
	{
		unsigned long long val = (unsigned long long) Heap_ExtractMinimum(&testheap);
		if(idx)
		{
			ck_assert(lastval <= val);
		}
		lastval = val;
	}

	ck_assert(0 == Heap_GetSize(&testheap));
	Heap_Destroy(&testheap);
}
END_TEST

START_TEST(heap_memoryleak_check)
{
	ck_assert(toutstanding_allocs() == 0);
}
END_TEST

Suite* heap_test_suite(void)
{
	Suite* s = suite_create("heap");
	TCase* testcases = tcase_create("Core");

	tcase_add_test(testcases, heap_create);
	tcase_add_test(testcases, heap_insert);
	tcase_add_test(testcases, heap_memoryleak_check);

	suite_add_tcase(s, testcases);

	return s;
}

int main(void)
{
	size_t tests_failed = 0;
	srand(time(NULL));
	Suite* s = heap_test_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	tests_failed = srunner_ntests_failed(sr);

	srunner_free(sr);

	talloc_subsys_release();
	return (tests_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
