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

enum cam_type_e
{
	CamType_Unknown = -1,
	CamType_Pi = 0,
	CamType_V4l2,
};


class CCam
{
protected:
	enum cam_type_e		m_cam_type;
	uint32_t			m_width;
	uint32_t			m_height;

	bool				m_terminate;
	bool				m_do_capture;

	MMAL_COMPONENT_T*	m_component_camera;
	MMAL_COMPONENT_T*	m_component_splitter;

public:

protected:

public:
	CCam();

	virtual ~CCam() = 0;

	enum cam_type_e GetCamType() {return m_cam_type;};

	virtual bool Init(const char* dev_name = NULL) = 0;

	virtual bool Setup(uint32_t width, uint32_t height) = 0;

	virtual bool StartCapture() = 0;

	virtual bool StopCapture() = 0;
};
#endif
