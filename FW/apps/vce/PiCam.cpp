#include "bcm_host.h"
#include "interface/vcos/vcos.h"

#include "PiCam.h"

//------------------------------------------------------------------------------------------------------
void CPiCam::preview_set_defaults()
{
	m_preview_parameters.wantPreview = 1;
	m_preview_parameters.wantFullScreenPreview = 1;
	m_preview_parameters.opacity = 255;
	m_preview_parameters.previewWindow.x = 0;
	m_preview_parameters.previewWindow.y = 0;
	m_preview_parameters.previewWindow.width = 1024;
	m_preview_parameters.previewWindow.height = 768;
	m_preview_parameters.preview_component = NULL;
	m_preview_parameters.display_num = -1;
}

void CPiCam::control_set_defaults()
{
	m_camera_parameters.sharpness = 0;
	m_camera_parameters.contrast = 0;
	m_camera_parameters.brightness = 50;
	m_camera_parameters.saturation = 0;
	m_camera_parameters.ISO = 0;                    // 0 = auto
	m_camera_parameters.videoStabilisation = 0;
	m_camera_parameters.exposureCompensation = 0;
	m_camera_parameters.exposureMode = MMAL_PARAM_EXPOSUREMODE_AUTO;
	m_camera_parameters.flickerAvoidMode = MMAL_PARAM_FLICKERAVOID_OFF;
	m_camera_parameters.exposureMeterMode = MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE;
	m_camera_parameters.awbMode = MMAL_PARAM_AWBMODE_AUTO;
	m_camera_parameters.imageEffect = MMAL_PARAM_IMAGEFX_NONE;
	m_camera_parameters.colourEffects.enable = 0;
	m_camera_parameters.colourEffects.u = 128;
	m_camera_parameters.colourEffects.v = 128;
	m_camera_parameters.rotation = 0;
	m_camera_parameters.hflip = m_camera_parameters.vflip = 0;
//	m_camera_parameters.roi.x = m_camera_parameters.roi.y = 0.0;
//	m_camera_parameters.roi.w = m_camera_parameters.roi.h = 1.0;
	m_camera_parameters.shutter_speed = 0;          // 0 = auto
	m_camera_parameters.awb_gains_r = 0;      // Only have any function if AWB OFF is used.
	m_camera_parameters.awb_gains_b = 0;
	m_camera_parameters.drc_level = MMAL_PARAMETER_DRC_STRENGTH_OFF;
	m_camera_parameters.stats_pass = MMAL_FALSE;
	m_camera_parameters.enable_annotate = 0;
	m_camera_parameters.annotate_string[0] = '\0';
	m_camera_parameters.annotate_text_size = 0;	//Use firmware default
	m_camera_parameters.annotate_text_colour = -1;   //Use firmware default
	m_camera_parameters.annotate_bg_colour = -1;     //Use firmware default
	m_camera_parameters.stereo_mode.mode = MMAL_STEREOSCOPIC_MODE_NONE;
	m_camera_parameters.stereo_mode.decimate = MMAL_FALSE;
	m_camera_parameters.stereo_mode.swap_eyes = MMAL_FALSE;
}

void CPiCam::commonsettings_set_defaults()
{
	memset(&m_common_settings, 0x0, sizeof(m_common_settings));

	strncpy(m_common_settings.camera_name, "(Unknown)", MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN);
	// We dont set width and height since these will be specific to the app being built.
	m_common_settings.width = 0;
	m_common_settings.height = 0;
	m_common_settings.filename = NULL;
	m_common_settings.verbose = 0;
	m_common_settings.cameraNum = 0;
	m_common_settings.sensor_mode = 0;
	m_common_settings.gps = 0;
}

void CPiCam::default_status()
{
	commonsettings_set_defaults();

	// Now set anything non-zero
	m_timeout = -1; // replaced with 5000ms later if unset
	m_common_settings.width = 1920;       // Default to 1080p
	m_common_settings.height = 1080;
	m_encoding = MMAL_ENCODING_H264;
	m_bitrate = 17000000; // This is a decent default bitrate for 1080p
	m_framerate = VIDEO_FRAME_RATE_NUM;
	m_intraperiod = -1;    // Not set
	m_quantisationParameter = 0;
	m_demoMode = 0;
	m_demoInterval = 250; // ms
	m_immutableInput = 1;
	m_profile = MMAL_VIDEO_PROFILE_H264_HIGH;
	m_level = MMAL_VIDEO_LEVEL_H264_4;
	m_waitMethod = WAIT_METHOD_NONE;
	m_onTime = 5000;
	m_offTime = 5000;
	m_bCapturing = 0;
	m_bInlineHeaders = 0;
	m_segmentSize = 0;  // 0 = not segmenting the file.
	m_segmentNumber = 1;
	m_segmentWrap = 0; // Point at which to wrap segment number back to 1. 0 = no wrap
	m_splitNow = 0;
	m_splitWait = 0;
	m_inlineMotionVectors = 0;
	m_intra_refresh_type = -1;
	m_frame = 0;
	m_save_pts = 0;
	m_netListen = false;
	m_addSPSTiming = MMAL_FALSE;
	m_slices = 1;

	preview_set_defaults();

	control_set_defaults();
}

void CPiCam::get_sensor_defaults(int camera_num, char *camera_name, int *width, int *height)
{
	MMAL_COMPONENT_T *camera_info;
	MMAL_STATUS_T status;

	// Default to the OV5647 setup
	strncpy(camera_name, "OV5647", MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN);

	// Try to get the camera name and maximum supported resolution
	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA_INFO, &camera_info);
	if (status == MMAL_SUCCESS)
	{
		MMAL_PARAMETER_CAMERA_INFO_T param;
		param.hdr.id = MMAL_PARAMETER_CAMERA_INFO;
		param.hdr.size = sizeof(param)-4;  // Deliberately undersize to check firmware version
		status = mmal_port_parameter_get(camera_info->control, &param.hdr);

		if (status != MMAL_SUCCESS)
		{
			// Running on newer firmware
			param.hdr.size = sizeof(param);
			status = mmal_port_parameter_get(camera_info->control, &param.hdr);
			if (status == MMAL_SUCCESS && param.num_cameras > camera_num)
			{
				// Take the parameters from the first camera listed.
				if (*width == 0)
				   *width = param.cameras[camera_num].max_width;
				if (*height == 0)
				   *height = param.cameras[camera_num].max_height;
				strncpy(camera_name, param.cameras[camera_num].camera_name, MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN);
				camera_name[MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN-1] = 0;
			}
			else
				vcos_log_error("Cannot read camera info, keeping the defaults for OV5647");
		}
		else
		{
			// Older firmware
			// Nothing to do here, keep the defaults for OV5647
		}

		mmal_component_destroy(camera_info);
	}
	else
	{
		vcos_log_error("Failed to create camera_info component");
	}

	// default to OV5647 if nothing detected..
	if (*width == 0)
	  *width = 2592;
	if (*height == 0)
	  *height = 1944;
}

void CPiCam::check_camera_model(int cam_num)
{
	MMAL_COMPONENT_T *camera_info;
	MMAL_STATUS_T status;

	// Try to get the camera name
	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA_INFO, &camera_info);
	if (status == MMAL_SUCCESS)
	{
		MMAL_PARAMETER_CAMERA_INFO_T param;
		param.hdr.id = MMAL_PARAMETER_CAMERA_INFO;
		param.hdr.size = sizeof(param)-4;  // Deliberately undersize to check firmware version
		status = mmal_port_parameter_get(camera_info->control, &param.hdr);

		if (status != MMAL_SUCCESS)
		{
			// Running on newer firmware
			param.hdr.size = sizeof(param);
			status = mmal_port_parameter_get(camera_info->control, &param.hdr);
			if (status == MMAL_SUCCESS && param.num_cameras > cam_num)
			{
				if (!strncmp(param.cameras[cam_num].camera_name, "toshh2c", 7))
				{
					fprintf(stderr, "The driver for the TC358743 HDMI to CSI2 chip you are using is NOT supported.\n");
					fprintf(stderr, "They were written for a demo purposes only, and are in the firmware on an as-is\n");
					fprintf(stderr, "basis and therefore requests for support or changes will not be acted on.\n\n");
				}
			}
		}

		mmal_component_destroy(camera_info);
	}
}

void CPiCam::default_camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{

}

MMAL_STATUS_T CPiCam::create_camera_component()
{
	MMAL_COMPONENT_T *camera = 0;
	MMAL_ES_FORMAT_T *format;
	MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;
	MMAL_STATUS_T status;
	int ret = 0;

	/* Create the component */
	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("Failed to create camera component");
		goto error;
	}

	ret = 	control_set_stereo_mode(camera->output[0], &m_camera_parameters.stereo_mode);
	ret += 	control_set_stereo_mode(camera->output[1], &m_camera_parameters.stereo_mode);
	ret += 	control_set_stereo_mode(camera->output[2], &m_camera_parameters.stereo_mode);

	if (ret != 0)
	{
		vcos_log_error("Could not set stereo mode : error %d", status);
		goto error;
	}

	MMAL_PARAMETER_INT32_T camera_num;

	camera_num.hdr.id = MMAL_PARAMETER_CAMERA_NUM;
	camera_num.hdr.size = sizeof(camera_num);
	camera_num.value = m_common_settings.cameraNum;

	status = mmal_port_parameter_set(camera->control, &camera_num.hdr);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("Could not select camera : error %d", status);
		goto error;
	}

	if (!camera->output_num)
	{
		status = MMAL_ENOSYS;
		vcos_log_error("Camera doesn't have output ports");
		goto error;
	}

	status = mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG, m_common_settings.sensor_mode);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("Could not set sensor mode : error %d", status);
		goto error;
	}

	preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
	video_port 	 = camera->output[MMAL_CAMERA_VIDEO_PORT];
	still_port 	 = camera->output[MMAL_CAMERA_CAPTURE_PORT];

	// Enable the camera, and tell it its control callback function
	status = mmal_port_enable(camera->control, default_camera_control_callback);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("Unable to enable control port : error %d", status);
		goto error;
	}

	//  set up the camera configuration
	{
		MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
		{
			{ MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
			.max_stills_w = (uint32_t)m_common_settings.width,
			.max_stills_h = (uint32_t)m_common_settings.height,
			.stills_yuv422 = 0,
			.one_shot_stills = 0,
			.max_preview_video_w = (uint32_t)m_common_settings.width,
			.max_preview_video_h = (uint32_t)m_common_settings.height,
			.num_preview_video_frames = (uint32_t)(3 + vcos_max(0, (m_framerate-30)/10)),
			.stills_capture_circular_buffer_height = 0,
			.fast_preview_resume = 0,
			.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RAW_STC
		};

		mmal_port_parameter_set(camera->control, &cam_config.hdr);
	}

	// Now set up the port formats

	// ------------------- Preview Port---------------------------------------------------------------
	// Set the encode format on the Preview port
	// HW limitations mean we need the preview to be the same size as the required recorded output
	format = preview_port->format;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->encoding_variant = MMAL_ENCODING_I420;

	if(m_camera_parameters.shutter_speed > 6000000)
	{
	  MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
			  	  	  	  	  	  	  	  	  { 50, 1000 }, {166, 1000}
	  	  	  	  	  	  	  	  	  	  	  };
	  mmal_port_parameter_set(preview_port, &fps_range.hdr);
	}
	else if(m_camera_parameters.shutter_speed > 1000000)
	{
		MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
												{ 166, 1000 }, {999, 1000}
												};
		mmal_port_parameter_set(preview_port, &fps_range.hdr);
	}

	//enable dynamic framerate if necessary
	if (m_camera_parameters.shutter_speed)
	{
		if (m_framerate > 1000000./ m_camera_parameters.shutter_speed)
		{
			m_framerate=0;
			fprintf(stderr, "Enable dynamic frame rate to fulfill shutter speed requirement\n");
		}
	}

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->es->video.width = VCOS_ALIGN_UP(m_common_settings.width, 32);
	format->es->video.height = VCOS_ALIGN_UP(m_common_settings.height, 16);
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = m_common_settings.width;
	format->es->video.crop.height = m_common_settings.height;
	format->es->video.frame_rate.num = m_framerate;
	format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;

	status = mmal_port_format_commit(preview_port);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("camera viewfinder format couldn't be set");
		goto error;
	}

	// ------------------- Encoder Port---------------------------------------------------------------
	// Set the encode format on the video  port

	format = video_port->format;
	format->encoding_variant = MMAL_ENCODING_I420;

	if(m_camera_parameters.shutter_speed > 6000000)
	{
		MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
		{ 50, 1000 }, {166, 1000}
		};
		mmal_port_parameter_set(video_port, &fps_range.hdr);
	}
	else if(m_camera_parameters.shutter_speed > 1000000)
	{
		MMAL_PARAMETER_FPS_RANGE_T fps_range = {{MMAL_PARAMETER_FPS_RANGE, sizeof(fps_range)},
		{ 167, 1000 }, {999, 1000}
		};
		mmal_port_parameter_set(video_port, &fps_range.hdr);
	}

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->es->video.width = VCOS_ALIGN_UP(m_common_settings.width, 32);
	format->es->video.height = VCOS_ALIGN_UP(m_common_settings.height, 16);
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = m_common_settings.width;
	format->es->video.crop.height = m_common_settings.height;
	format->es->video.frame_rate.num = m_framerate;
	format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;

	status = mmal_port_format_commit(video_port);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("camera video format couldn't be set");
		goto error;
	}

	// Ensure there are enough buffers to avoid dropping frames
	if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
		video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

	// ------------------- Still Port---------------------------------------------------------------
	// Set the encode format on the still  port
	format = still_port->format;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->encoding_variant = MMAL_ENCODING_I420;

	format->es->video.width = VCOS_ALIGN_UP(m_common_settings.width, 32);
	format->es->video.height = VCOS_ALIGN_UP(m_common_settings.height, 16);
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = m_common_settings.width;
	format->es->video.crop.height = m_common_settings.height;
	format->es->video.frame_rate.num = 0;
	format->es->video.frame_rate.den = 1;

	status = mmal_port_format_commit(still_port);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("camera still format couldn't be set");
		goto error;
	}

	/* Ensure there are enough buffers to avoid dropping frames */
	if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
		still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

	/* Enable component */
	status = mmal_component_enable(camera);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("camera component couldn't be enabled");
		goto error;
	}

	// Note: this sets lots of parameters that were not individually addressed before.
	control_set_all_parameters(camera, &m_camera_parameters);

	m_camera_component = camera;

//	update_annotation_data(state);

	fprintf(stderr, "Camera component done\n");

	return status;

error:

   if (camera)
	  mmal_component_destroy(camera);

	return status;
}

void CPiCam::destroy_camera_component()
{
	if (m_camera_component)
	{
		mmal_component_destroy(m_camera_component);
		m_camera_component = NULL;
	}
}













//------------------------------------------------------------------------------------------------------
CPiCam::CPiCam()
{
	m_status = MMAL_SUCCESS;

	bcm_host_init();

	default_status();
}

CPiCam::~CPiCam()
{

}

bool CPiCam::Init()
{
	bool ret = false;

	get_sensor_defaults(m_common_settings.cameraNum, m_common_settings.camera_name, &m_common_settings.width, &m_common_settings.height);

	check_camera_model(m_common_settings.cameraNum);

	if ((m_status = create_camera_component()) != MMAL_SUCCESS)
	{
		vcos_log_error("%s: Failed to create preview component", __func__);
		destroy_camera_component();
//		exit_code = EX_SOFTWARE;
	}

	return (ret = (m_status == MMAL_SUCCESS) ? true : false);
}
