#include "charvector.h"
#include <stdio.h>
#include <stdlib.h>


int main(void)
{
	cv_t str;
	cv_init(&str, 1);
	cv_sprintf(&str, "Meow %d\r\n", 5);
	printf(str.data);
	cv_destroy(&str);
	tprint_summary();
	return 0;
}
