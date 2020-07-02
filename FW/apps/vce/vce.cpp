#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bcm_host.h"
#include "VceCtrl.h"
#include <msg_broker.h>

static int is_terminate = 0;
static CVceCtrl* vce_ctrl = NULL;

static void process_message(MsgContext* msg, void* user_data)
{
	if (vce_ctrl)
		vce_ctrl->ProcessMessage(msg, user_data);
}

static void default_signal_handler(int signal_number)
{
	switch(signal_number)
	{
	case SIGINT:
	case SIGTERM:
		is_terminate = 1;
		break;
	default:
		fprintf(stderr, "%s: %d\n", __func__, signal_number);
		break;
	}
}

int main(int argc, char* argv[])
{
	bcm_host_init();

	if (!vce_ctrl)
		vce_ctrl = new CVceCtrl();

	signal(SIGINT, default_signal_handler);
	signal(SIGTERM, default_signal_handler);

	MsgBroker_Run("/tmp/fifo/vce.fifo", process_message, NULL, &is_terminate);

	if (vce_ctrl)
	{
		delete vce_ctrl;
		vce_ctrl = NULL;
	}

	return 0;
}
