#ifndef __COMPONENT_H__
#define __COMPONENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

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

#ifdef __cplusplus
}
#endif

//-----------------------------------------------------------------
typedef enum
{
	MMAL_COMPNENT_UNKNOWN = -1,
	MMAL_COMPNENT_CAMERA,
	MMAL_COMPNENT_H264Enc,
	MMAL_COMPNENT_MJPEGEnc,
	MMAL_COMPNENT_VideoSplitter,
}MMAL_COMPNENT_TYPE_E;
//-----------------------------------------------------------------

class CComponent;

typedef struct component_port_info_s
{
	uint32_t			idx;
	MMAL_PORT_T* 		port;
	MMAL_POOL_T* 		pool;
	MMAL_CONNECTION_T	connect;
	CComponent*			obj;
}component_port_info_t;

class CComponent
{
protected:
	MMAL_COMPNENT_TYPE_E	m_component_type;
	MMAL_COMPONENT_T*		m_component;
	component_port_info_t 	m_input;
	component_port_info_t* 	m_output;
public:

protected:

	MMAL_STATUS_T Create(const char* name);

	MMAL_STATUS_T Destroy();

	void DumpMmalPortFormat(MMAL_ES_FORMAT_T* format);

	void DumpMmalPort(MMAL_PORT_T* port);

public:
	CComponent();

	~CComponent();

	MMAL_STATUS_T CreateCamera();

	MMAL_STATUS_T CreateH264Encoder();

	MMAL_STATUS_T CreateMJPEGEncoder();

	MMAL_STATUS_T CreateVideoSplitter(MMAL_FOURCC_T fourcc, uint32_t width, uint32_t height, uint32_t stride, uint32_t input_buffer_num);

	MMAL_STATUS_T Enable();
};
#endif
