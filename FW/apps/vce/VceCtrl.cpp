#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "VceCtrl.h"
#include "PiCam.h"
#include "V4l2Cam.h"

CVceCtrl::CVceCtrl()
{
	memset(m_camera, 0x0, sizeof(CCam*) * MAX_CAMERA_NUM);

	SetupUpCameara();
}

CVceCtrl::~CVceCtrl()
{
	for (int i=0 ; i<MAX_CAMERA_NUM ; ++i)
	{
		if (m_camera[i])
		{
			delete m_camera[i];
			m_camera[i] = NULL;
		}
	}
}

void CVceCtrl::ProcessMessage(MsgContext* msg, void* user_data)
{

}

bool CVceCtrl::SetupUpCameara()
{
	int i;
	bool ret = true;
	enum cam_type_e cam_type = CamType_Pi;

	for (i=0 ; i<MAX_CAMERA_NUM ; ++i)
	{
		cam_type = i == 0 ? CamType_Unknown : CamType_V4l2;

		if (!m_camera[i])
		{
			if (cam_type == CamType_Pi)
				m_camera[i] = new CPiCam();
			else if (cam_type == CamType_V4l2)
				m_camera[i] = new CV4l2Cam();
			else
				fprintf(stderr, "Unknown Camera Type\n");

			if (m_camera[i])
				ret = m_camera[i]->Init(i, "/dev/video0");
		}
	}


	return ret;
}
