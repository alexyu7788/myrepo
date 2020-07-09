#ifndef __CAMERA_H__
#define __CAMERA_H__

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

#include "common.h"
// ---------------------------------------------------------------------------------
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
// ---------------------------------------------------------------------------------
enum cam_type_e
{
	CamType_Unknown = -1,
	CamType_Pi = 0,
	CamType_V4l2,
};
// ---------------------------------------------------------------------------------
struct port_info;
typedef void (*PFUNC_RETURNBUFFERSTOPORT)(struct port_info* port_info);
// ---------------------------------------------------------------------------------
struct port_info
{
	uint32_t					idx;
	class CCam* 				cam_obj;
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
class CCam
{
protected:
	int					m_id;
	enum cam_type_e		m_cam_type;
	uint32_t			m_width;
	uint32_t			m_height;
	unsigned char		m_num_planes;

	bool				m_terminate;
	bool				m_do_capture;

	MMAL_COMPONENT_T*	m_component_camera;
	MMAL_COMPONENT_T*	m_component_splitter;

public:

protected:
	void DumpMmalPortFormat(MMAL_ES_FORMAT_T* format);

	void DumpMmalPort(MMAL_PORT_T* port);

	MMAL_STATUS_T	CreateComponent(struct component& component, const char* name);

	MMAL_STATUS_T	DestroyComponent(struct component& component);

	MMAL_STATUS_T	SetupComponentVideoInput(struct component& component,
													MMAL_FOURCC_T fourcc,
													uint32_t x,
													uint32_t y,
													uint32_t w,
													uint32_t h,
													uint32_t buf_num,
													void* user_data,
													MMAL_PORT_BH_CB_T in_cb);

	MMAL_STATUS_T	SetupComponentVideoOutput(struct component& component,
													MMAL_ES_FORMAT_T* format,
													uint32_t buf_num,
													PFUNC_RETURNBUFFERSTOPORT pfunc,
													MMAL_PORT_BH_CB_T out_cb);

	MMAL_STATUS_T	EnableComponentPorts(struct component& component, MMAL_PORT_BH_CB_T in_cb, MMAL_PORT_BH_CB_T out_cb);




public:
	CCam();

	virtual ~CCam();

	enum cam_type_e GetCamType() {return m_cam_type;};

	virtual bool Init(int id, const char* dev_name = NULL);

	virtual bool Setup(uint32_t width, uint32_t height);

	virtual bool StartCapture();

	virtual bool StopCapture();
};
#endif
