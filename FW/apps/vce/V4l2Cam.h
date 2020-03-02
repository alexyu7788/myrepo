#ifndef __V4l2CAMERA_H__
#define __V4l2CAMERA_H__

#include "Camera.h"

class CV2l2Cam : public CCam
{
protected:

public:

protected:

public:
	CV2l2Cam();

	~CV2l2Cam();

	bool Init(enum cam_type_e cam_type);
};
#endif
