#include "Camera.h"

CCam::CCam()
{
	m_cam_type 	= CamType_Unknown;
	m_width 	= 0;
	m_height	= 0;

	m_terminate = false;
	m_do_capture= false;
}

CCam::~CCam()
{

}

