#include "charvector.h"
#include <stdio.h>
#include <stdlib.h>


int main(void)
{
	cv_t str;
	cv_init(&str, 1);
//	cv_append(&str, "to u", 5);
	size_t idx = 0, z = 0;
	for(; idx < 10; ++idx)
		cv_push(&str, idx);

	char nums[] = {10, 11, 12, 13, 14};
	cv_append(&str, nums, 5);

	for(idx = 0, z = cv_len(&str); idx < z; ++idx)
		printf(idx < z - 1 ? "%d," : "%d\n", cv_at(&str, idx));

//	printf("%s\r\n", str.data);
	cv_destroy(&str);
	tprint_summary();
	return 0;
}
