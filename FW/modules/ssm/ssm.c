#include "ssm.h"

static void ssm_init(void)    __attribute__((constructor));
static void ssm_release(void) __attribute__((destructor));

static void ssm_init(void)
{
	// Initialize VCOS
	vcos_init();

	// Start the shared memory support.
	if ( vcsm_init() == -1 )
	{
		fprintf(stderr, "Cannot initialize vcsm device\n" );
	}
}

static void ssm_release(void)
{
	// Terminate the shared memory support.
	vcsm_exit ();
}

int ssm_test()
{
	unsigned int hldr;

	hldr = vcsm_malloc_cache(20, VCSM_CACHE_TYPE_HOST, "ssm test");

	return 0;
}
