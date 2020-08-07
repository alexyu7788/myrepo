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

#include "component.h"
#ifdef __cplusplus
}
#endif

#include "common.h"
#include "Encoder.h"
// ---------------------------------------------------------------------------------
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
// ---------------------------------------------------------------------------------
enum cam_type_e
{
	CamType_Unknown = -1,
	CamType_Pi = 0,
	CamType_V4l2,
};
//// ---------------------------------------------------------------------------------
//struct port_info;
//typedef void (*PFUNC_RETURNBUFFERSTOPORT)(struct port_info* port_info);
//// ---------------------------------------------------------------------------------
//struct port_info
//{
//	uint32_t					idx;
//	class CCam* 				cam_obj;
//	MMAL_PORT_T* 				port;
//	MMAL_POOL_T* 				pool;
//	MMAL_CONNECTION_T*			connect;
//	PFUNC_RETURNBUFFERSTOPORT	return_buf_to_port;
//};
//
//struct component {
//	MMAL_COMPONENT_T*	comp;
//	struct port_info  	input;
//	struct port_info* 	output;
//};
//// ---------------------------------------------------------------------------------
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

	struct component	m_video_source;
public:

protected:

public:
	friend class CEncoder;

	CCam();

	virtual ~CCam();

	enum cam_type_e GetCamType() {return m_cam_type;};

	virtual bool Init(int id, const char* dev_name = NULL);

	virtual bool Setup(uint32_t width, uint32_t height);

	virtual bool StartCapture();

	virtual bool StopCapture();
};
#endif
