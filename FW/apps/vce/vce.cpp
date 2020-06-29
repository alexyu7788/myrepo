#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bcm_host.h"
#include "VceCtrl.h"
#include <msg_broker.h>

static CVceCtrl* vce_ctrl = NULL;

int main(int argc, char* argv[])
{
	bcm_host_init();

	if (!vce_ctrl)
		vce_ctrl = new CVceCtrl();

	sleep(1);

	if (vce_ctrl)
	{
		delete vce_ctrl;
		vce_ctrl = NULL;
	}

	bcm_host_deinit();

	return 0;
}
