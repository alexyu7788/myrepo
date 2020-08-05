#include "Encoder.h"
#include "PiCam.h"

// ---------------------------------------------------------------------------------

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

	encoder_input 	= m_encoder.comp->input[0];
	encoder_output 	= m_encoder.comp->output[0];

	// We want same format on input and output
	src_output = m_src->m_video_source.comp->input[0];
	status = mmal_format_full_copy(encoder_input->format, src_output->format);

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


	status = mmal_component_enable(m_encoder.comp);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Unable to enable %s.\n" PRINTF_COLOR_NONE, __func__, m_encoder.comp->name);
		goto fail;
	}

	/* Create pool of buffer headers for the output port to consume */
	m_encoder.output[0].pool = mmal_port_pool_create(encoder_output, encoder_output->buffer_num, encoder_output->buffer_size);
	if (!m_encoder.output[0].pool)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Failed to create buffer header pool for encoder output port %s.\n" PRINTF_COLOR_NONE, __func__, encoder_output->name);
	}

	m_encoder.input.port 		= m_encoder.comp->input[0];
	m_encoder.output[0].port 	= m_encoder.comp->output[0];

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

	return status;
}

MMAL_STATUS_T CEncoder::Stop()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	return status;
}
