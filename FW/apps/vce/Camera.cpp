#include "Camera.h"

CCam::CCam()
{
	m_cam_type 	= CamType_Unknown;
	m_width 	= 0;
	m_height	= 0;

	m_terminate = false;
	m_do_capture= false;

	/* Components */
	m_component_camera		= NULL;
	m_component_splitter	= NULL;
}

CCam::~CCam()
{
	if (m_component_splitter)
	{
		mmal_component_destroy(m_component_splitter);
		m_component_splitter = NULL;
	}

	if (m_component_camera)
	{
		mmal_component_destroy(m_component_camera);
		m_component_camera = NULL;
	}
}

