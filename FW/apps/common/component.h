#ifndef __COMPONENT_H__
#define __COMPONENT_H__

#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/mmal_parameters_camera.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "interface/vmcs_host/vc_vchi_gencmd.h"

#include "common.h"
// ---------------------------------------------------------------------------------
struct port_info;
typedef void (*PFUNC_RETURNBUFFERSTOPORT)(struct port_info* port_info);
// ---------------------------------------------------------------------------------
struct port_info
{
	uint32_t					idx;
	MMAL_PORT_T* 				port;
	MMAL_POOL_T* 				pool;
	MMAL_CONNECTION_T*			connect;
	PFUNC_RETURNBUFFERSTOPORT	return_buf_to_port;
};

struct component {
	MMAL_COMPONENT_T*	comp;
	struct port_info  	input;
	struct port_info* 	output;
};
// ---------------------------------------------------------------------------------
void DumpMmalPort(MMAL_PORT_T* port);
void DumpMmalPortFormat(MMAL_ES_FORMAT_T* format);

MMAL_STATUS_T	CreateComponent(struct component* component, const char* name);
MMAL_STATUS_T	DestroyComponent(struct component* component);
MMAL_STATUS_T	SetupComponentVideoInput(struct component* component,
													MMAL_FOURCC_T fourcc,
													uint32_t x,
													uint32_t y,
													uint32_t w,
													uint32_t h,
													uint32_t buf_num,
													void* user_data,
													MMAL_PORT_BH_CB_T in_cb);
MMAL_STATUS_T	SetupComponentVideoOutput(struct component* component,
													MMAL_ES_FORMAT_T* format,
													uint32_t buf_num,
													PFUNC_RETURNBUFFERSTOPORT pfunc,
													MMAL_PORT_BH_CB_T out_cb);
MMAL_STATUS_T	EnableComponentPorts(struct component* component, MMAL_PORT_BH_CB_T in_cb, MMAL_PORT_BH_CB_T out_cb);
#endif
