#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <stdio.h>
#include <stdlib.h>

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

public:

protected:

public:
	CCam();

	virtual ~CCam() = 0;

	enum cam_type_e GetCamType() {return m_cam_type;};

	virtual bool Init(enum cam_type_e cam_type) = 0;
};
#endif
