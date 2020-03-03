#include "ssm.h"

int ssm_test()
{
	unsigned int hldr;

	hldr = vcsm_malloc_cache(20, VCSM_CACHE_TYPE_HOST, "ssm test");

	return 0;
}
