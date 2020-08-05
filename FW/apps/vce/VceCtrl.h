#ifndef __VceCtrl_H__
#define __VceCtrl_H__

#include <msg_broker.h>
#include "Camera.h"
#include "Encoder.h"

#define MAX_CAMERA_NUM	2
#define MAX_ENCODER_NUM	3

// ---------------------------------------------------------------------------------
class CVceCtrl
{
protected:

	CCam* m_camera[MAX_CAMERA_NUM];
	CEncoder* m_encoder[MAX_CAMERA_NUM * ENC_TYPE_MAX];

public:

protected:

public:

	CVceCtrl();

	~CVceCtrl();

	void ProcessMessage(MsgContext*, void* user_data);

	bool SetupUpCameara();

	bool SetupEncoder();
};
#endif
