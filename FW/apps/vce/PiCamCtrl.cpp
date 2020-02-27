#include "PiCam.h"

// Control

int CPiCam::control_set_all_parameters(MMAL_COMPONENT_T *camera, const RASPICAM_CAMERA_PARAMETERS *params)
{
	int result;


	return result;
}




int CPiCam::control_set_stereo_mode(MMAL_PORT_T *port, MMAL_PARAMETER_STEREOSCOPIC_MODE_T *stereo_mode)
{
	MMAL_PARAMETER_STEREOSCOPIC_MODE_T stereo = { {MMAL_PARAMETER_STEREOSCOPIC_MODE, sizeof(stereo)},
												MMAL_STEREOSCOPIC_MODE_NONE, MMAL_FALSE, MMAL_FALSE};

	if (stereo_mode->mode != MMAL_STEREOSCOPIC_MODE_NONE)
	{
		stereo.mode = stereo_mode->mode;
		stereo.decimate = stereo_mode->decimate;
		stereo.swap_eyes = stereo_mode->swap_eyes;
	}

	return mmal_status_to_int(mmal_port_parameter_set(port, &stereo.hdr));
}
