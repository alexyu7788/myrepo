#include "Component.h"






//-----------------------------------------------------------------
MMAL_STATUS_T CComponent::Create(const char* name)
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	status = mmal_component_create(name, &m_component);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, "Failed to create video splitter component\n");
		return status;
	}

	fprintf(stderr, "[%s] %s has been created with id %d, in %d, out %d\n", __func__,
			m_component->name,
			m_component->id,
			m_component->input_num,
			m_component->output_num);


	return status;
}

MMAL_STATUS_T CComponent::Destroy()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	if (!m_output)
		return MMAL_SUCCESS;

	mmal_pool_destroy(m_input.pool);

	if (m_output)
	{
		for (size_t i=0 ; i<(sizeof(m_output)/sizeof(m_output[0])) ; ++i)
			mmal_pool_destroy(m_output[i].pool);

		free (m_output);
		m_output = NULL;
	}

	if (m_component)
	{
		fprintf(stderr, "[%s]: %s\n", __func__, m_component->name);

		status = mmal_component_destroy(m_component);
		m_component = NULL;
	}

	return status;
}

void CComponent::DumpMmalPortFormat(MMAL_ES_FORMAT_T* format)
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

	fprintf(stderr, "[%s] type: %s, fourcc: %4.4s\n", __func__, name_type, (char *)&format->encoding);
	fprintf(stderr, "[%s] bitrate: %i, framed: %i\n", __func__, format->bitrate, !!(format->flags & MMAL_ES_FORMAT_FLAG_FRAMED));
	fprintf(stderr, "[%s] extra data: %i, %p\n", __func__, format->extradata_size, format->extradata);

	switch(format->type)
	{
		case MMAL_ES_TYPE_AUDIO:
			fprintf(stderr, "[%s] samplerate: %i, channels: %i, bps: %i, block align: %i\n", __func__,
			format->es->audio.sample_rate, format->es->audio.channels,
			format->es->audio.bits_per_sample, format->es->audio.block_align);
			break;

		case MMAL_ES_TYPE_VIDEO:
			fprintf(stderr, "[%s] width: %i, height: %i, (%i,%i,%i,%i)\n", __func__,
			format->es->video.width, format->es->video.height,
			format->es->video.crop.x, format->es->video.crop.y,
			format->es->video.crop.width, format->es->video.crop.height);

			fprintf(stderr, "[%s] pixel aspect ratio: %i/%i, frame rate: %i/%i\n", __func__,
			format->es->video.par.num, format->es->video.par.den,
			format->es->video.frame_rate.num, format->es->video.frame_rate.den);
			break;

		case MMAL_ES_TYPE_SUBPICTURE:
			break;

		default:
			break;
	}
}

void CComponent::DumpMmalPort(MMAL_PORT_T* port)
{
	if (!port)
		return;

	fprintf(stderr, "[%s] MMAL port info of %s(%p)\n", __func__, port->name, port);

	DumpMmalPortFormat(port->format);

	fprintf(stderr, "[%s] buffers num: %i(opt %i, min %i), size: %i(opt %i, min: %i), align: %i\n", __func__,
			port->buffer_num, port->buffer_num_recommended, port->buffer_num_min,
			port->buffer_size, port->buffer_size_recommended, port->buffer_size_min,
			port->buffer_alignment_min);
}
//-----------------------------------------------------------------
CComponent::CComponent()
{
	m_component_type = MMAL_COMPNENT_UNKNOWN;
	m_component = NULL;
	m_output	= NULL;

	memset(&m_input, 0x0, sizeof(m_input));
}

CComponent::~CComponent()
{
	Destroy();
}

MMAL_STATUS_T CComponent::CreateCamera()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	status = Create(MMAL_COMPONENT_DEFAULT_CAMERA);
	if (status != MMAL_SUCCESS)
		goto error;

	return status;
error:

	return Destroy();
}

MMAL_STATUS_T CComponent::CreateH264Encoder()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	status = Create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER);
	if (status != MMAL_SUCCESS)
		goto error;

	return status;
error:

	return Destroy();
}

MMAL_STATUS_T CComponent::CreateMJPEGEncoder()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	status = Create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER);
	if (status != MMAL_SUCCESS)
		goto error;

	return status;
error:

	return Destroy();
}

MMAL_STATUS_T CComponent::CreateVideoSplitter(MMAL_FOURCC_T fourcc, uint32_t width, uint32_t height, uint32_t stride, uint32_t input_buffer_num)
{
	MMAL_STATUS_T status = MMAL_SUCCESS;
	MMAL_ES_FORMAT_T* format;
	unsigned int mmal_stride;

	status = Create(MMAL_COMPONENT_DEFAULT_VIDEO_SPLITTER);
	if (status != MMAL_SUCCESS)
		goto error;

	if (!m_component->input_num)
	{
		status = MMAL_ENOSYS;
		fprintf(stderr, "[%s] %s doesn't have any input port\n", __func__, m_component->name);
		goto error;
	}

	if (m_component->output_num < 2)
	{
		status = MMAL_ENOSYS;
		fprintf(stderr, "[%s] %s doesn't have enough output ports", __func__, m_component->name);
		goto error;
	}

	m_output = (component_port_info_t*)malloc(sizeof(component_port_info_t) * m_component->output_num);
	if (!m_output)
	{
		status = MMAL_ENOMEM;
		fprintf(stderr, "[%s] Can not allocate memory for %d output\n", __func__, m_component->name, m_component->output_num);
		goto error;
	}

	/* Setup Format of Splitter Input */
	format = m_component->input[0]->format;

	format->type	 				= MMAL_ES_TYPE_VIDEO;
	format->encoding 				= fourcc;
	format->es->video.crop.x 		= 0;
	format->es->video.crop.y 		= 0;
	format->es->video.crop.width 	= width;
	format->es->video.crop.height 	= height;
	format->es->video.width 		= VCOS_ALIGN_UP(format->es->video.crop.width, 32);
	format->es->video.height 		= VCOS_ALIGN_UP(format->es->video.crop.height, 16);

	m_component->input[0]->buffer_num = input_buffer_num;

	status = mmal_port_format_commit(m_component->input[0]);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, "[%s] Unable to set format on %s input port\n", __func__, m_component->name);
		goto error;
	}

	DumpMmalPort(m_component->input[0]);

	mmal_stride = mmal_encoding_width_to_stride(fourcc, format->es->video.width);
	fprintf(stderr, "[%s] stride %d/ bytesperline %d\n", __func__, mmal_stride, stride);

	/* Setup Pool of Splitter Input */
	m_input.idx  		= 0;
	m_input.port 		= m_component->input[0];
	m_input.pool 		= mmal_pool_create(m_input.port->buffer_num, 0);
	if (!m_input.pool)
	{
		fprintf(stderr, "[%s] Failed to create buffer header pool for splitter input port %s", __func__, m_input.port->name);
		goto error;
	}

	fprintf(stderr, "[%s] Created pool of length %d of size %d for %s\n", __func__, m_input.port->buffer_num, 0, m_input.port->name);
	m_input.port->userdata = (struct MMAL_PORT_USERDATA_T *)this;	/* Used for callback of input port. */

	/* Setup Splitter Output */
	for (uint32_t i=0 ; i<m_component->output_num ; ++i)
	{
		m_output[i].idx  		= i;
		m_output[i].port 		= m_component->output[i];
		mmal_format_copy(m_output[i].port->format, m_input.port->format);

		m_output[i].port->format->encoding = m_input.port->format->encoding;//MMAL_ENCODING_I420;
		m_output[i].port->buffer_num = 3;	// why???

//		format = m_splitter.component->output[i].port->format;
//		format->es->video.crop.x 		= 0;
//		format->es->video.crop.y 		= 0;
//		format->es->video.crop.width 	= 800;
//		format->es->video.crop.height 	= 600;
//		format->es->video.width 		= VCOS_ALIGN_UP(format->es->video.crop.width, 32);
//		format->es->video.height 		= VCOS_ALIGN_UP(format->es->video.crop.height, 16);

		status = mmal_port_format_commit(m_output[i].port);

		if (status != MMAL_SUCCESS)
		{
			fprintf(stderr, "[%s] Unable to set format on %s output port[%d]\n", __func__, m_component->name, i);
			goto error;
		}

		fprintf(stderr, "[%s] m_splitter.output_port[%d]'s format->video.size now %dx%d\n", __func__, i,
				m_output[i].port->format->es->video.width, m_output[i].port->format->es->video.height);

		m_output[i].port->userdata = (MMAL_PORT_USERDATA_T*)&m_output[i];/* Used for callback of output port. */

		status = mmal_port_parameter_set_boolean(m_output[i].port, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);

//		status = mmal_port_enable(m_output[i].port, Splitter_Outputput_Port_CB);
		if (status != MMAL_SUCCESS)
		{
			fprintf(stderr, "[%s] %s couldn't enabled.\n", __func__, m_output[i].port->name);
			goto error;
		}

//		m_splitter.output_port->buffer_num = m_splitter.output_port->buffer_num_min;
//		if (m_splitter.output_port->buffer_num < m_splitter.output_port->buffer_num_min)
//			m_splitter.output_port->buffer_num = m_splitter.output_port->buffer_num_min;
//
//		m_splitter.output_port->buffer_size = m_splitter.output_port->buffer_size_recommended;
//		if (m_splitter.output_port->buffer_size < m_splitter.output_port->buffer_size_min)
//			m_splitter.output_port->buffer_size = m_splitter.output_port->buffer_size_min;

		fprintf(stderr, "[%s] Create port pool of %d buffers of size %d for %s\n", __func__,
				m_output[i].port->buffer_num, m_output[i].port->buffer_size, m_output[i].port->name);

		m_output[i].pool = mmal_port_pool_create(m_output[i].port, m_output[i].port->buffer_num, m_output[i].port->buffer_size);
		if (!m_output[i].pool)
		{
			fprintf(stderr, "[%s] Failed to create buffer header pool for splitter output port %s", __func__, m_output[i].port->name);
			goto error;
		}
//
//		m_output[i].return_buf_to_port = ReturnBuffersToPort;
//		m_soutput[i].return_buf_to_port(&m_splitter.output[i]);
	}

	return status;

error:

	return Destroy();
}

MMAL_STATUS_T CComponent::Enable()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	if (m_component)
		status = mmal_component_enable(m_component);

	return status;
}
