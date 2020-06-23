#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "VceCtrl.h"
#include <msg_broker.h>

static CVceCtrl* vce_ctrl = NULL;

int main(int argc, char* argv[])
{
	if (!vce_ctrl)
		vce_ctrl = new CVceCtrl();

	sleep(3);

	if (vce_ctrl)
	{
		delete vce_ctrl;
		vce_ctrl = NULL;
	}

	return 0;
}
