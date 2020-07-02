#ifndef __VceCtrl_H__
#define __VceCtrl_H__

#include <msg_broker.h>
#include "Camera.h"

#define MAX_CAMERA_NUM	2

class CVceCtrl
{
protected:

	CCam* m_camera[MAX_CAMERA_NUM];

public:

protected:

public:

	CVceCtrl();

	~CVceCtrl();

	void ProcessMessage(MsgContext*, void* user_data);

	bool SetupUpCameara();
};
#endif
