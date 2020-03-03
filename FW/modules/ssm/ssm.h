#ifndef __SSM_H__
#define __SSM_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include "user-vcsm.h"
#include "interface/vcos/vcos.h"
#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_buffer.h"

typedef struct ssm_buffer_s
{
	int idx;
	MMAL_BUFFER_HEADER_T bh;
}ssm_buffer_t;

int ssm_test();


#endif
