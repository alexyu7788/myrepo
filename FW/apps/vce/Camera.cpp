#include "Camera.h"

// ---------------------------------------------------------------------------------
#define CHECK_STATUS(status, msg) if (status != MMAL_SUCCESS) { fprintf(stderr, msg"\n"); goto error; }
// ---------------------------------------------------------------------------------
void CCam::DumpMmalPortFormat(MMAL_ES_FORMAT_T* format)
{
	const char *name_type;

	if (!format)
	return;

	switch(format->type)
	{
		case MMAL_ES_TYPE_AUDIO: name_type = "audio"; break;
		case MMAL_ES_TYPE_VIDEO: name_type = "video"; break;
		case MMAL_ES_TYPE_SUBPICTURE: name_type = "subpicture"; break;
		default: name_type = "unknown"; break;
	}

	fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] type: %s, fourcc: %4.4s\n" PRINTF_COLOR_NONE, __func__, name_type, (char *)&format->encoding);
	fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] bitrate: %i, framed: %i\n" PRINTF_COLOR_NONE, __func__, format->bitrate, !!(format->flags & MMAL_ES_FORMAT_FLAG_FRAMED));
	fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] extra data: %i, %p\n" PRINTF_COLOR_NONE, __func__, format->extradata_size, format->extradata);

	switch(format->type)
	{
		case MMAL_ES_TYPE_AUDIO:
			fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] samplerate: %i, channels: %i, bps: %i, block align: %i\n" PRINTF_COLOR_NONE, __func__,
			format->es->audio.sample_rate, format->es->audio.channels,
			format->es->audio.bits_per_sample, format->es->audio.block_align);
			break;

		case MMAL_ES_TYPE_VIDEO:
			fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] width: %i, height: %i, (%i,%i,%i,%i)\n" PRINTF_COLOR_NONE, __func__,
			format->es->video.width, format->es->video.height,
			format->es->video.crop.x, format->es->video.crop.y,
			format->es->video.crop.width, format->es->video.crop.height);

			fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] pixel aspect ratio: %i/%i, frame rate: %i/%i\n" PRINTF_COLOR_NONE, __func__,
			format->es->video.par.num, format->es->video.par.den,
			format->es->video.frame_rate.num, format->es->video.frame_rate.den);
			break;

		case MMAL_ES_TYPE_SUBPICTURE:
			break;

		default:
			break;
	}
}

void CCam::DumpMmalPort(MMAL_PORT_T* port)
{
	if (!port)
		return;

	fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] MMAL port info of %s(%p)\n" PRINTF_COLOR_NONE, __func__, port->name, port);

	DumpMmalPortFormat(port->format);

	fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] buffers num: %i(opt %i, min %i), size: %i(opt %i, min: %i), align: %i\n" PRINTF_COLOR_NONE, __func__,
			port->buffer_num, port->buffer_num_recommended, port->buffer_num_min,
			port->buffer_size, port->buffer_size_recommended, port->buffer_size_min,
			port->buffer_alignment_min);
}

MMAL_STATUS_T CCam::CreateComponent(struct component& component, const char* name)
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	status = mmal_component_create(name, &component.comp);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to create component %s\n" PRINTF_COLOR_NONE, __func__, name);
		goto error;
	}

	fprintf(stderr, PRINTF_COLOR_BROWN "[%s] Created component %s with %d input & %d output(s)\n" PRINTF_COLOR_NONE, __func__, name, component.comp->input_num, component.comp->output_num);

	if (!component.comp->input_num)
	{
//		status = MMAL_ENOSYS;
		fprintf(stderr, PRINTF_COLOR_LIGHT_RED "[%s] %s doesn't have any input port\n" PRINTF_COLOR_NONE, __func__, component.comp->name);
//		goto error;
	}
	else
	{
//		memset(&component.input, 0x0, sizeof(port_info));
//
//		component.input.port = component.comp->input[0];
	}

	if (component.comp->output_num < 1)
	{
//		status = MMAL_ENOSYS;
		fprintf(stderr, PRINTF_COLOR_LIGHT_RED "[%s] %s doesn't have enough output ports\n" PRINTF_COLOR_NONE, __func__, component.comp->name);
//		goto error;
	}
	else
	{
		component.output = (struct port_info*)malloc(sizeof(port_info) * component.comp->output_num);
		if (!component.output)
		{
			status = MMAL_ENOMEM;
			fprintf(stderr, PRINTF_COLOR_RED "[%s] Can not allocate memory for %d output\n" PRINTF_COLOR_NONE, __func__, component.comp->name, component.comp->output_num);
			goto error;
		}

//		for (size_t idx = 0 ; idx < component.comp->output_num ; ++idx)
//		{
//			memset(&component.output[idx], 0x0, sizeof(struct port_info));
//
//			component.output[idx].idx	= idx;
//			component.output[idx].port	= component.comp->output[idx];
//		}
	}

	return status;

error:

	DestroyComponent(component);

	return status;
}


MMAL_STATUS_T CCam::DestroyComponent(struct component& component)
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	if (component.output)
	{
		for (size_t idx = 0 ; idx < component.comp->output_num ; ++idx)
		{
			if (component.output[idx].pool)
			{
				mmal_port_pool_destroy(component.output[idx].port, component.output[idx].pool);
				component.output[idx].pool = NULL;
			}
		}

		free (component.output);
		component.output = NULL;
	}

	if (component.input.pool)
	{
		mmal_port_pool_destroy(component.input.port, component.input.pool);
		component.input.pool = NULL;
	}

	status = mmal_component_destroy(component.comp);
	component.comp = NULL;

	return status;
}

MMAL_STATUS_T CCam::SetupComponentVideoInput(struct component& component,
		MMAL_FOURCC_T fourcc,
		uint32_t x,
		uint32_t y,
		uint32_t w,
		uint32_t h,
		uint32_t buf_num,
		void* user_data,
		MMAL_PORT_BH_CB_T in_cb)
{
	MMAL_STATUS_T status = MMAL_SUCCESS;
	MMAL_ES_FORMAT_T* format;

	format = component.comp->input[0]->format;

	format->type 					= MMAL_ES_TYPE_VIDEO;
	format->encoding				= fourcc;
	format->es->video.crop.x 		= x;
	format->es->video.crop.y 		= y;
	format->es->video.crop.width 	= w;
	format->es->video.crop.height 	= h;
	format->es->video.width 		= VCOS_ALIGN_UP(format->es->video.crop.width, 32);
	format->es->video.height 		= VCOS_ALIGN_UP(format->es->video.crop.height, 16);

	component.comp->input[0]->buffer_num = buf_num;

	status = mmal_port_format_commit(component.comp->input[0]);

	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set format on %s input port\n" PRINTF_COLOR_NONE, __func__, component.comp->name);
		goto error;
	}

	/* Setup Pool of Splitter Input */
	component.input.idx  		= 0;
//	component.input.cam_obj 	= this;
	component.input.port 		= component.comp->input[0];
	component.input.pool 		= mmal_pool_create(component.input.port->buffer_num, 0);
	if (!component.input.pool)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to create buffer header pool for splitter input port %s\n" PRINTF_COLOR_NONE, __func__, component.input.port->name);
		goto error;
	}

	fprintf(stderr, PRINTF_COLOR_GREEN "[%s] Created pool of length %d of size %d for %s\n" PRINTF_COLOR_NONE, __func__, component.input.port->buffer_num, 0, component.input.port->name);
	component.input.port->userdata = (struct MMAL_PORT_USERDATA_T *)user_data;	/* Used for callback of input port. */


	mmal_port_parameter_set_boolean(component.input.port, MMAL_PARAMETER_ZERO_COPY, MMAL_FALSE);
	status = mmal_port_enable(component.input.port, in_cb);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] %s couldn't enabled.\n" PRINTF_COLOR_NONE, __func__, component.input.port->name);
		goto error;
	}

	fprintf(stderr, PRINTF_COLOR_CYAN "[%s] %s is enabled.\n" PRINTF_COLOR_NONE, __func__, component.input.port->name);

	DumpMmalPort(component.input.port);

	return status;

error:

	DestroyComponent(component);
	return status;
}

MMAL_STATUS_T CCam::SetupComponentVideoOutput(struct component& component,
											  	  MMAL_ES_FORMAT_T* format,
												  uint32_t buf_num,
												  PFUNC_RETURNBUFFERSTOPORT pfunc,
												  MMAL_PORT_BH_CB_T out_cb)
{
	MMAL_STATUS_T status = MMAL_SUCCESS;
	MMAL_PORT_T	*out_port;
	MMAL_ES_FORMAT_T *in_format, *out_format;

	in_format = component.input.port->format;

	for (uint32_t idx = 0 ; idx < component.comp->output_num; ++idx)
	{
		component.output[idx].idx 	= idx;
//		component.output[idx].cam_obj = this;
		out_port 	= component.output[idx].port = component.comp->output[idx];
		out_format 	= out_port->format;

		mmal_format_copy(out_format, in_format);
		out_format->encoding = in_format->encoding;//MMAL_ENCODING_I420
		out_port->buffer_num = 3;//why?

		status = mmal_port_format_commit(out_port);
		if (status != MMAL_SUCCESS)
		{
			fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set format on %s output port[%d]\n" PRINTF_COLOR_NONE, __func__, component.comp->name, idx);
			goto error;
		}

		fprintf(stderr, PRINTF_COLOR_CYAN "[%s] %s's format->video.size now %dx%d\n" PRINTF_COLOR_NONE, __func__, out_port->name,
				out_format->es->video.width, out_format->es->video.height);

		out_port->userdata = (MMAL_PORT_USERDATA_T*)&component.output[idx];/* Used for callback of output port. */

		status = mmal_port_parameter_set_boolean(out_port, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);

		// Enable output port
		status = mmal_port_enable(out_port, out_cb);
		if (status != MMAL_SUCCESS)
		{
			fprintf(stderr, PRINTF_COLOR_RED "[%s] %s couldn't enabled.\n" PRINTF_COLOR_NONE, __func__, out_port->name);
			goto error;
		}

		fprintf(stderr, PRINTF_COLOR_CYAN "[%s] Create port pool of %d buffers of size %d for %s\n" PRINTF_COLOR_NONE, __func__,
				out_port->buffer_num, out_port->buffer_size, out_port->name);

		component.output[idx].pool = mmal_port_pool_create(out_port, out_port->buffer_num, out_port->buffer_size);
		if (!component.output[idx].pool)
		{
			fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to create buffer header pool for splitter output port %s.\n" PRINTF_COLOR_NONE, __func__, out_port->name);
			goto error;
		}

		if (pfunc)
		{
			component.output[idx].return_buf_to_port = pfunc;
			pfunc(&component.output[idx]);
		}

		DumpMmalPort(out_port);
	}

	return status;

error:

	DestroyComponent(component);
	return status;
}

MMAL_STATUS_T CCam::EnableComponentPorts(struct component& component, MMAL_PORT_BH_CB_T in_cb, MMAL_PORT_BH_CB_T out_cb)
{
	MMAL_STATUS_T status = MMAL_SUCCESS;
	MMAL_PORT_T	*in_port, *out_port;

//	for (uint32_t idx = 0 ; idx < component.comp->output_num; ++idx)
//	{
//		out_port = component.output[idx].port;
//		status = mmal_port_enable(out_port, out_cb);
//		if (status != MMAL_SUCCESS)
//		{
//			fprintf(stderr, PRINTF_COLOR_RED "[%s] %s couldn't enabled.\n" PRINTF_COLOR_NONE, __func__, out_port->name);
//			goto error;
//		}
//
//		fprintf(stderr, PRINTF_COLOR_CYAN "[%s] %s is enabled.\n" PRINTF_COLOR_NONE, __func__, out_port->name);
//	}

	in_port = component.input.port;

	mmal_port_parameter_set_boolean(in_port, MMAL_PARAMETER_ZERO_COPY, MMAL_FALSE);
	status = mmal_port_enable(in_port, in_cb);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] %s couldn't enabled.\n" PRINTF_COLOR_NONE, __func__, in_port->name);
		goto error;
	}

	fprintf(stderr, PRINTF_COLOR_CYAN "[%s] %s is enabled.\n" PRINTF_COLOR_NONE, __func__, in_port->name);

	return status;

error:

	DestroyComponent(component);
	return status;
}









// ---------------------------------------------------------------------------------
CCam::CCam()
{
	m_id		= -1;
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

bool CCam::Init(int id, const char* dev_name)
{
	return true;
}

bool CCam::Setup(uint32_t width, uint32_t height)
{
	return true;
}

bool CCam::StartCapture()
{
	return true;
}

bool CCam::StopCapture()
{
	return true;
}
