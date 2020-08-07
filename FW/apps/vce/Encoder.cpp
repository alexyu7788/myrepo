#include "Encoder.h"
#include "PiCam.h"

// ---------------------------------------------------------------------------------
void CEncoder::InputPort_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	CEncoder* pThis = (CEncoder*)port->userdata;

	if (buffer->length)
		fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] buffer header's pts %lld ms.\n" PRINTF_COLOR_NONE, __func__, buffer->pts);

	mmal_buffer_header_release(buffer);
}

void CEncoder::OutputPort_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	MMAL_STATUS_T status;

	CEncoder* pThis = (CEncoder*)port->userdata;

	if (!pThis)
		return;

	fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] buffer header's len %u Bytes. flag 0x%X\n" PRINTF_COLOR_NONE, __func__, buffer->length >> 3, buffer->flags);

	if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_KEYFRAME)
		fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] buffer is key frame\n" PRINTF_COLOR_NONE, __func__);
//	else if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_FRAME_END)
//		fprintf(stderr, PRINTF_COLOR_YELLOW "[%s] buffer is frame end\n" PRINTF_COLOR_NONE, __func__);

	buffer->length = 0;
	status = mmal_port_send_buffer(pThis->m_encoder.comp->output[0], buffer);
	mmal_buffer_header_release(buffer);
}

void CEncoder::ControlPort_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	fprintf(stderr, "Camera control callback  cmd=0x%08x\n", buffer->cmd);

	if (buffer->cmd == MMAL_EVENT_PARAMETER_CHANGED)
	{
		MMAL_EVENT_PARAMETER_CHANGED_T *param = (MMAL_EVENT_PARAMETER_CHANGED_T *)buffer->data;
		switch (param->hdr.id)
		{
			case MMAL_PARAMETER_CAMERA_SETTINGS:
			{
				MMAL_PARAMETER_CAMERA_SETTINGS_T *settings = (MMAL_PARAMETER_CAMERA_SETTINGS_T*)param;
				fprintf(stderr, "Exposure now %u, analog gain %u/%u, digital gain %u/%u",
				settings->exposure,
				settings->analog_gain.num, settings->analog_gain.den,
				settings->digital_gain.num, settings->digital_gain.den);
				fprintf(stderr, "AWB R=%u/%u, B=%u/%u",
				settings->awb_red_gain.num, settings->awb_red_gain.den,
				settings->awb_blue_gain.num, settings->awb_blue_gain.den);
			}
			break;
		}
	}
	else if (buffer->cmd == MMAL_EVENT_ERROR)
	{
		fprintf(stderr, "No data received from sensor. Check all connections, including the Sunny one on the camera board");
	}
	else
	{
		fprintf(stderr, "Received unexpected camera control callback event, 0x%08x", buffer->cmd);
	}

	mmal_buffer_header_release(buffer);
}
// ---------------------------------------------------------------------------------
CEncoder::CEncoder()
{
	m_id 		= 0;
	m_width 	= 0;
	m_height 	= 0;

	m_src		= NULL;

	memset(&m_encoder, 0x0, sizeof(m_encoder));
	memset(&m_splitter, 0x0, sizeof(m_splitter));

	m_connect 	= NULL;

	m_enc_type  = ENC_TYPE_UNKNOWN;
	m_encoding  = 0;
	m_level		= 0;
	m_bitrate	= 0;
	m_profile	= 0;
	m_intraperiod = -1;
	m_quantisationParameter = 0;

}

CEncoder::~CEncoder()
{
	DestroyComponent(&m_encoder);
	DestroyComponent(&m_splitter);
}

bool CEncoder::Init(int idx, class CCam* cam, MMAL_FOURCC_T encoding, int level, int bitrate)
{
	bool ret = true;
	MMAL_STATUS_T status = MMAL_SUCCESS;
	MMAL_PORT_T *encoder_input = NULL, *encoder_output = NULL, *src_output = NULL;

	if (!cam)
		return false;

	m_src 		= cam;

	m_encoding 	= encoding;
	m_level 	= level;
	m_bitrate 	= bitrate;

	status = CreateComponent(&m_encoder, MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER);
	if (status != MMAL_SUCCESS)
	{
		ret = false;
		goto fail;
	}

	if (!m_encoder.comp->input_num || !m_encoder.comp->output_num)
	{
		status = MMAL_ENOSYS;
		ret = false;
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Video encoder doesn't have input/output ports" PRINTF_COLOR_NONE, __func__);
		goto fail;
	}

	// Setup input of encoder.
	encoder_input 	= m_encoder.comp->input[0];

	src_output = m_src->m_video_source.comp->output[0];
	status = mmal_format_full_copy(encoder_input->format, src_output->format);
	encoder_input->buffer_num = 3;
	if (status == MMAL_SUCCESS)
		status = mmal_port_format_commit(encoder_input);

	status = mmal_port_parameter_set_boolean(encoder_input, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set Zero-Copy on %s .\n" PRINTF_COLOR_NONE, __func__, encoder_input->name);
		goto fail;
	}

	encoder_input->userdata = (struct MMAL_PORT_USERDATA_T *)this;

	// Setup output of encoder.
	encoder_output 	= m_encoder.comp->output[0];
	encoder_output->format->encoding =m_encoding;

	if(m_encoding == MMAL_ENCODING_H264)
	{
		if(m_level == MMAL_VIDEO_LEVEL_H264_4)
		{
			if(m_bitrate > MAX_BITRATE_LEVEL4)
			{
				fprintf(stderr, "Bitrate too high: Reducing to 25MBit/s\n");
				m_bitrate = MAX_BITRATE_LEVEL4;
			}
		}
		else
		{
			if(m_bitrate > MAX_BITRATE_LEVEL42)
			{
				fprintf(stderr, "Bitrate too high: Reducing to 62.5MBit/s\n");
				m_bitrate = MAX_BITRATE_LEVEL42;
			}
		}
	}
	else if(m_encoding == MMAL_ENCODING_MJPEG)
	{
		if(m_bitrate > MAX_BITRATE_MJPEG)
		{
			fprintf(stderr, "Bitrate too high: Reducing to 25MBit/s\n");
			m_bitrate = MAX_BITRATE_MJPEG;
		}
	}

	encoder_output->format->bitrate = m_bitrate;

	if (m_encoding == MMAL_ENCODING_H264)
		encoder_output->buffer_size = encoder_output->buffer_size_recommended;
	else
		encoder_output->buffer_size = 256<<10;

	if (encoder_output->buffer_size < encoder_output->buffer_size_min)
		encoder_output->buffer_size = encoder_output->buffer_size_min;

	encoder_output->buffer_num = encoder_output->buffer_num_recommended;

	if (encoder_output->buffer_num < encoder_output->buffer_num_min)
		encoder_output->buffer_num = encoder_output->buffer_num_min;

	encoder_output->buffer_num = 8;

	// We need to set the frame rate on output to 0, to ensure it gets
	// updated correctly from the input framerate when port connected
	encoder_output->format->es->video.frame_rate.num = 0;
	encoder_output->format->es->video.frame_rate.den = 1;

	// Commit the port changes to the output port
	status = mmal_port_format_commit(encoder_output);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set format on %s output port.\n" PRINTF_COLOR_NONE, __func__, m_encoder.comp->name);
		goto fail;
	}

	status = mmal_port_parameter_set_boolean(encoder_output, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set Zero-Copy on %s .\n" PRINTF_COLOR_NONE, __func__, encoder_output->name);
		goto fail;
	}

	/* Key frame rate */
	status = SetupIntraPeroid();
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set IntraPeroid on %s output port.\n" PRINTF_COLOR_NONE, __func__, m_encoder.comp->name);
		goto fail;
	}

	/* QP */
	status = SetupQP();
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set QP on %s output port.\n" PRINTF_COLOR_NONE, __func__, m_encoder.comp->name);
		goto fail;
	}

	status = mmal_port_parameter_set_boolean(encoder_output, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set Zero-Copy on %s .\n" PRINTF_COLOR_NONE, __func__, encoder_output->name);
		goto fail;
	}






	/* Enable input port */
	status = mmal_port_enable(encoder_input, InputPort_CB);
	if (status != MMAL_SUCCESS)
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to enable %s.\n" PRINTF_COLOR_NONE, __func__,
				encoder_input->name);

	/* Create pool of buffer headers for the input port to consume */
//	encoder_input->buffer_size = 0;
	printf("%s: %d/%d\n", encoder_input->name, encoder_input->buffer_num, 0/*encoder_input->buffer_size*/);
	m_encoder.input.pool = mmal_port_pool_create(encoder_input, encoder_input->buffer_num, encoder_input->buffer_size);
	if (!m_encoder.input.pool)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to create buffer header pool for encoder input port %s.\n" PRINTF_COLOR_NONE, __func__, encoder_input->name);
	}

//	for (unsigned int i=0 ; i<encoder_input->buffer_num ; ++i)
//	{
//		MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(m_encoder.input.pool->queue);
//
//		if (!buffer)
//		{
//			fprintf(stderr, PRINTF_COLOR_RED "[%s] Where'd my buffer go?!.\n" PRINTF_COLOR_NONE, __func__);
//			goto fail;
//		}
//
//		status = mmal_port_send_buffer(encoder_input, buffer);
//		if(status != MMAL_SUCCESS)
//		{
//			fprintf(stderr, PRINTF_COLOR_RED "[%s] mmal_port_send_buffer failed on buffer %p, status %d\n" PRINTF_COLOR_NONE, __func__, buffer, status);
//			goto fail;
//		}
//		fprintf(stderr, PRINTF_COLOR_GREEN "[%s] Sent buffer %p\n" PRINTF_COLOR_NONE, __func__, buffer);
//	}

	/* Enable output port */
	encoder_output->userdata = (struct MMAL_PORT_USERDATA_T*)this;
	status = mmal_port_enable(encoder_output, OutputPort_CB);
	if (status != MMAL_SUCCESS)
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to enable %s.\n" PRINTF_COLOR_NONE, __func__,
				encoder_output->name);

	/* Create pool of buffer headers for the output port to consume */
	printf("%s: %d/%d\n", encoder_output->name, encoder_output->buffer_num, encoder_output->buffer_size);
	m_encoder.output[0].pool = mmal_port_pool_create(encoder_output, encoder_output->buffer_num, encoder_output->buffer_size);
	if (!m_encoder.output[0].pool)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to create buffer header pool for encoder output port %s.\n" PRINTF_COLOR_NONE, __func__, encoder_output->name);
	}

	for (unsigned int i=0 ; i<encoder_output->buffer_num ; ++i)
	{
		MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(m_encoder.output[0].pool->queue);

		if (!buffer)
		{
			fprintf(stderr, PRINTF_COLOR_RED "[%s] Where'd my buffer go?!.\n" PRINTF_COLOR_NONE, __func__);
			goto fail;
		}

		status = mmal_port_send_buffer(encoder_output, buffer);
		if(status != MMAL_SUCCESS)
		{
			fprintf(stderr, PRINTF_COLOR_RED "[%s] mmal_port_send_buffer failed on buffer %p, status %d\n" PRINTF_COLOR_NONE, __func__, buffer, status);
			goto fail;
		}
		fprintf(stderr, PRINTF_COLOR_GREEN "[%s] Sent buffer %p\n" PRINTF_COLOR_NONE, __func__, buffer);
	}

	m_encoder.input.port 		= m_encoder.comp->input[0];
	m_encoder.output[0].port 	= m_encoder.comp->output[0];

	m_src->m_video_source.output[0].conn_comp = m_encoder.comp;
	m_src->m_video_source.output[0].sink_pool = m_encoder.input.pool;

	/* Enable component */
	status = mmal_component_enable(m_encoder.comp);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to enable %s.\n" PRINTF_COLOR_NONE, __func__, m_encoder.comp->name);
		goto fail;
	}

	return ret;

fail:

	DestroyComponent(&m_encoder);
	DestroyComponent(&m_splitter);

	return ret;
}

MMAL_STATUS_T CEncoder::SetupIntraPeroid()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;
	MMAL_PORT_T* encoder_output = NULL;

	encoder_output = m_encoder.comp->output[0];

	if (m_encoding == MMAL_ENCODING_H264 && m_intraperiod != -1)
	{
		MMAL_PARAMETER_UINT32_T param = {{ MMAL_PARAMETER_INTRAPERIOD, sizeof(param)}, (uint32_t)m_intraperiod};
		status = mmal_port_parameter_set(encoder_output, &param.hdr);
		if (status != MMAL_SUCCESS)
			fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set intraperiod.\n" PRINTF_COLOR_NONE, __func__);
	}

	return status;
}
MMAL_STATUS_T CEncoder::SetupQP()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;
	MMAL_PORT_T* encoder_output = NULL;

	encoder_output = m_encoder.comp->output[0];

	if (m_encoding == MMAL_ENCODING_H264 && m_quantisationParameter)
	{
		MMAL_PARAMETER_UINT32_T param = {{ MMAL_PARAMETER_VIDEO_ENCODE_INITIAL_QUANT, sizeof(param)}, (uint32_t)m_quantisationParameter};
		status = mmal_port_parameter_set(encoder_output, &param.hdr);
		if (status != MMAL_SUCCESS)
			fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set initial QP.\n" PRINTF_COLOR_NONE, __func__);

		MMAL_PARAMETER_UINT32_T param2 = {{ MMAL_PARAMETER_VIDEO_ENCODE_MIN_QUANT, sizeof(param)}, (uint32_t)m_quantisationParameter};
		status = mmal_port_parameter_set(encoder_output, &param2.hdr);
		if (status != MMAL_SUCCESS)
			fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set min QP.\n" PRINTF_COLOR_NONE, __func__);

		MMAL_PARAMETER_UINT32_T param3 = {{ MMAL_PARAMETER_VIDEO_ENCODE_MAX_QUANT, sizeof(param)}, (uint32_t)m_quantisationParameter};
		status = mmal_port_parameter_set(encoder_output, &param3.hdr);
		if (status != MMAL_SUCCESS)
			fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to set max QP.\n" PRINTF_COLOR_NONE, __func__);
	}

	return status;
}

MMAL_STATUS_T CEncoder::SetupProfile()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	return status;
}

MMAL_STATUS_T CEncoder::Start()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	if (!m_src || !m_encoder.comp->input_num)
		return MMAL_EFAULT;

	if (m_src->m_cam_type == CamType_Pi)
	{
		status = mmal_connection_create(&m_connect, m_src->m_video_source.output[0].port, m_encoder.input.port, MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
		if (status == MMAL_SUCCESS)
		{
			status =  mmal_connection_enable(m_connect);
			if (status != MMAL_SUCCESS)
				mmal_connection_destroy(m_connect);
		}

		if (status != MMAL_SUCCESS)
		{
			m_connect = NULL;
			fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to connect %s to %s.\n" PRINTF_COLOR_NONE, __func__,
					m_src->m_video_source.output[0].port->name, m_encoder.input.port->name);
		}
	}
	else
	{
//		m_encoder.input.port->userdata = (struct MMAL_PORT_USERDATA_T*)this;
//		status = mmal_port_enable(m_encoder.input.port, InputPort_CB);
//		if (status != MMAL_SUCCESS)
//			fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to enable %s.\n" PRINTF_COLOR_NONE, __func__,
//					m_encoder.input.port->name);

//		m_encoder.output[0].port->userdata = (struct MMAL_PORT_USERDATA_T*)this;
//		status = mmal_port_enable(m_encoder.output[0].port, OutputPort_CB);
//		if (status != MMAL_SUCCESS)
//			fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to enable %s.\n" PRINTF_COLOR_NONE, __func__,
//					m_encoder.output[0].port->name);
	}

	return status;
}

MMAL_STATUS_T CEncoder::Stop()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	return status;
}
