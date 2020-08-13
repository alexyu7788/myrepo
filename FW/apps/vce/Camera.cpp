#include "Camera.h"

// ---------------------------------------------------------------------------------
#define CHECK_STATUS(status, msg) if (status != MMAL_SUCCESS) { fprintf(stderr, msg"\n"); goto error; }
// ---------------------------------------------------------------------------------
CCam::CCam()
{
	m_id		= -1;
	m_cam_type 	= CamType_Unknown;
	m_width 	= 0;
	m_height	= 0;
	m_fps		= 0;

	m_terminate = false;
	m_do_capture= false;

	memset(&m_video_source, 0x0, sizeof(m_video_source));
}

CCam::~CCam()
{

}

bool CCam::Init(int id, const char* dev_name)
{
	return true;
}

bool CCam::Setup(uint32_t width, uint32_t height)
{
	return true;
}

bool CCam::StartCapture()
{
	return true;
}

bool CCam::StopCapture()
{
	return true;
}
