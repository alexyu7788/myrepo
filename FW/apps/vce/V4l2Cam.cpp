#include "V4l2Cam.h"

CV2l2Cam::CV2l2Cam()
{

}

CV2l2Cam::~CV2l2Cam()
{

}

bool CV2l2Cam::Init(enum cam_type_e cam_type)
{

	m_cam_type = CamType_V4l2;

	return true;
}
