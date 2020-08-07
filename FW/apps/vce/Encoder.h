#ifndef __ENCODER_H__
#define __ENCODER_H__

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
#include "Camera.h"
// ---------------------------------------------------------------------------------
typedef enum
{
	ENC_TYPE_UNKNOWN = -1,
	ENC_TYPE_SRC = 0,
	ENC_TYPE_RTSP,
	ENC_TYPE_RTMP,
	ENC_TYPE_MAX,
}ENC_TYPE_E;
// ---------------------------------------------------------------------------------
class CEncoder
{
protected:
	int					m_id;
	uint32_t			m_width;
	uint32_t			m_height;

	class CCam*			m_src;
	struct component	m_encoder;
	struct component	m_splitter;
	/* MMAL */
	MMAL_CONNECTION_T*	m_connect;

	ENC_TYPE_E			m_enc_type;
	MMAL_FOURCC_T 		m_encoding;	/// Requested codec video encoding (MJPEG or H264)
	int					m_level;	/// H264 level to use for encoding
	int 				m_bitrate; 	/// Requested bitrate
	int					m_profile;	/// H264 profile to use for encoding
	int 				m_intraperiod;  /// Intra-refresh period (key frame rate)
	int 				m_quantisationParameter;          /// Quantisation parameter - quality. Set bitrate 0 and set this for variable bitrate


public:

protected:
	static void InputPort_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

	static void OutputPort_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

	static void ControlPort_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
public:
	friend class CCam;

	CEncoder();

	~CEncoder();

	bool Init(int idx, class CCam* cam, MMAL_FOURCC_T encoding, int level, int bitrate);

	MMAL_STATUS_T SetupIntraPeroid();

	MMAL_STATUS_T SetupQP();

	MMAL_STATUS_T SetupProfile();

	MMAL_STATUS_T Start();

	MMAL_STATUS_T Stop();
};







#endif /* __ENCODER_H__ */
