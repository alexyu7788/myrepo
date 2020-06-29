#include "V4l2Cam.h"

// ---------------------------------------------------------------------------------
int CV4l2Cam::XIoctl(int fd, int request, void *arg)
{
	int r;

	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}

bool CV4l2Cam::QueryCap()
{
	bool ret = true;
    char fourcc[5] = {0};
    char c, e;
    int support_grbg10 = 0;
    struct v4l2_crop crop;

	if (-1 == XIoctl(m_fd, VIDIOC_QUERYCAP, &m_caps))
	{
		perror("Querying Capabilities");
		return false;
	}

	printf( "Driver Caps:\n"
	                "  Driver: \"%s\"\n"
	                "  Card: \"%s\"\n"
	                "  Bus: \"%s\"\n"
	                "  Version: %d.%d\n"
	                "  Capabilities: %08x\n",
					m_caps.driver,
					m_caps.card,
					m_caps.bus_info,
	                (m_caps.version>>16)&&0xff,
	                (m_caps.version>>24)&&0xff,
					m_caps.capabilities);

	if (!(m_caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		fprintf(stderr, "%s is not a video capture capability.\n", m_dev_name);
		return false;
	}

	switch (m_io_method)
	{
	case IO_METHOD_READ:
		if (!(m_caps.capabilities & V4L2_CAP_READWRITE))
		{
			fprintf(stderr, "%s does not support read i/o.\n", m_dev_name);
			return false;
		}
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(m_caps.capabilities & V4L2_CAP_STREAMING))
		{
			fprintf(stderr, "%s does not support streaming.\n", m_dev_name);
			return false;
		}
		break;
	}

	// Cropcap
	m_cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == XIoctl (m_fd, VIDIOC_CROPCAP, &m_cropcap))
	{
		crop.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c 		= m_cropcap.defrect;

		if (-1 == XIoctl(m_fd, VIDIOC_S_CROP, &crop))
		{
            switch (errno) {
            case EINVAL:
                	/* Cropping not supported. */
            		fprintf(stderr, "%s does not support cropping.\n", m_dev_name);
                    break;
            default:
                    /* Errors ignored. */
            		fprintf(stderr, "%s: VIDIOC_S_CROP(%d,%s)\n", m_dev_name, errno, strerror(errno));
                    break;
            }
		}
	}
	else
	{
		perror("Querying Cropping Capabilities");
		return false;
	}

	printf( "Camera Cropping:\n"
			"  Bounds: %dx%d+%d+%d\n"
			"  Default: %dx%d+%d+%d\n"
			"  Aspect: %d/%d\n",
			m_cropcap.bounds.width, m_cropcap.bounds.height, m_cropcap.bounds.left, m_cropcap.bounds.top,
			m_cropcap.defrect.width, m_cropcap.defrect.height, m_cropcap.defrect.left, m_cropcap.defrect.top,
			m_cropcap.pixelaspect.numerator, m_cropcap.pixelaspect.denominator);

	// Format Description
    printf("FMT Desc:\n");
	m_fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (0 == XIoctl(m_fd, VIDIOC_ENUM_FMT, &m_fmtdesc))
    {
		strncpy(fourcc, (char *)&m_fmtdesc.pixelformat, 4);
		if (m_fmtdesc.pixelformat == V4L2_PIX_FMT_SGRBG10)
			support_grbg10 = 1;
		c = m_fmtdesc.flags & 1? 'C' : ' ';
		e = m_fmtdesc.flags & 2? 'E' : ' ';

		printf("  [%u] %s: %c%c %s\n", m_fmtdesc.index++, fourcc, c, e, m_fmtdesc.description);
    }

//    if (!support_grbg10)
//    {
//        printf("Doesn't support GRBG10.\n");
//        return false;
//    }

//    m_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    if ( -1 == XIoctl(m_fd, VIDIOC_G_FMT, &m_fmt))
//	{
//		perror("Querying VIDIOC_G_FMT");
//		return false;
//	}
//
//    printf("Get Format:\n");
//    struct v4l2_pix_format		pix;
//    memcpy(&pix, &m_fmt.fmt.pix, sizeof(pix));
//
//    strncpy(fourcc, (char *)&pix.pixelformat, 4);
//
//    printf("%u %ux%u, pixelformat %s, field %u, byteperline %u, sizeimage %u, colorspace %u, flags 0x%X\n",
//    		m_fmt.type,
//			pix.width, pix.height,
//			fourcc,
//			pix.field,
//			pix.bytesperline,
//			pix.sizeimage,
//			pix.colorspace,
//			pix.flags);

	return ret;
}

bool CV4l2Cam::InitMemRead(unsigned int buffer_size)
{
	bool  ret = true;

	return ret;
}

bool CV4l2Cam::InitMemMmap()
{
	bool  ret = true;
	struct v4l2_requestbuffers req;

	memset(&req, 0x0, sizeof(req));

	req.count 	= 4;
	req.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory 	= V4L2_MEMORY_MMAP;

	if (-1 == XIoctl(m_fd, VIDIOC_REQBUFS, &req))
	{
		switch (errno)
		{
		case EINVAL:
			fprintf(stderr, "[%s] does not support memory mapping.\n", m_dev_name);
			ret = false;
			break;
		default:
			fprintf(stderr, "[%s] VIDIOC_REQBUFS: %d, %s\n", m_dev_name, errno, strerror(errno));
			ret = false;
			break;
		}
	}

	if (req.count < 2)
	{
		fprintf(stderr, "[%s] Insufficient buffer memory.\n", m_dev_name);
		ret = false;
	}

	if (ret == false)
	{
		fprintf(stderr, "[%s][%s] VIDIOC_REQBUFS failed.\n", m_dev_name, __func__);
		return ret;
	}

	m_buffers = (struct buffer*)calloc(req.count, sizeof(*m_buffers));
	if (!m_buffers)
	{
		fprintf(stderr, "[%s][%s]: Out of memory.\n", m_dev_name, __func__);
		return false;
	}

	for (m_n_buffer = 0 ; m_n_buffer < req.count ; ++m_n_buffer)
	{
		struct v4l2_buffer	buf;

		memset(&buf, 0x0, sizeof(buf));

		buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory 	= V4L2_MEMORY_MMAP;
		buf.index	= m_n_buffer;

		if (-1 == XIoctl(m_fd, VIDIOC_QUERYBUF, &buf))
		{
			fprintf(stderr, "[%s][%s] VIDIOC_QUERYBUF failed.\n", m_dev_name, __func__);
			ret = false;
			break;
		}

		m_buffers[m_n_buffer].length = buf.length;
		m_buffers[m_n_buffer].start = (struct buffer*)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, buf.m.offset);

		if (MAP_FAILED == m_buffers[m_n_buffer].start)
		{
			fprintf(stderr, "[%s][%s] MMAP failed.\n", m_dev_name, __func__);
			ret = false;
			break;
		}

		fprintf(stderr, "[%s][%s] MMAP [%d]: %p, %d.\n", m_dev_name, __func__, m_n_buffer, m_buffers[m_n_buffer].start, m_buffers[m_n_buffer].length);
	}

	return ret;
}

bool CV4l2Cam::InitMemUserPtr(unsigned int buffer_size)
{
	bool  ret = true;
	struct v4l2_requestbuffers req;

	memset(&req, 0x0, sizeof(req));

	req.count 	= 4;
	req.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory 	= V4L2_MEMORY_USERPTR;

	if (-1 == XIoctl(m_fd, VIDIOC_REQBUFS, &req))
	{
		switch (errno)
		{
		case EINVAL:
			fprintf(stderr, "[%s] does not support memory mapping.\n", m_dev_name);
			ret = false;
			break;
		default:
			fprintf(stderr, "[%s] VIDIOC_REQBUFS: %d, %s\n", m_dev_name, errno, strerror(errno));
			ret = false;
			break;
		}
	}

	m_buffers = (struct buffer*)calloc(req.count, sizeof(*m_buffers));
	if (!m_buffers)
	{
		fprintf(stderr, "[%s][%s]: Out of memory.\n", m_dev_name, __func__);
		return false;
	}

	for (m_n_buffer = 0 ; m_n_buffer < req.count ; ++m_n_buffer)
	{
		m_buffers[m_n_buffer].length = buffer_size;
		m_buffers[m_n_buffer].start  = malloc(buffer_size);

		if (!m_buffers[m_n_buffer].start)
		{
			fprintf(stderr, "[%s][%s] UserPtr failed.\n", m_dev_name, __func__);
			ret = false;
			break;
		}

		fprintf(stderr, "[%s][%s] UserPtr [%d]: %p, %d.\n", m_dev_name, __func__, m_n_buffer, m_buffers[m_n_buffer].start, m_buffers[m_n_buffer].length);
	}

	return ret;
}

bool CV4l2Cam::InitMem(unsigned int buffer_size)
{
	bool  ret = true;

	switch (m_io_method)
	{
	case IO_METHOD_READ:
		ret = InitMemRead(buffer_size);
		break;
	case IO_METHOD_MMAP:
		ret = InitMemMmap();
		break;
	case IO_METHOD_USERPTR:
		ret = InitMemUserPtr(buffer_size);
		break;
	}

	return ret;
}

void CV4l2Cam::DeInitMem()
{
	switch (m_io_method)
	{
	case IO_METHOD_READ:
		fprintf(stderr, "[%s][%s] IO_METHOD_READ.\n", m_dev_name, __func__);
		if (m_buffers[0].start)
			free (m_buffers[0].start);
		break;
	case IO_METHOD_MMAP:
		fprintf(stderr, "[%s][%s] IO_METHOD_MMAP.\n", m_dev_name, __func__);
		for (uint32_t i=0 ; i < m_n_buffer ; ++i)
		{
			if (m_buffers[i].start)
			{
				if (-1 == munmap(m_buffers[i].start, m_buffers[i].length))
				{
					fprintf(stderr, "[%s][%s] munmap failed.(%d, %s)\n", m_dev_name, __func__, errno, strerror(errno));
				}
			}
		}
		break;
	case IO_METHOD_USERPTR:
		fprintf(stderr, "[%s][%s] IO_METHOD_USERPTR.\n", m_dev_name, __func__);
		for (uint32_t i=0 ; i < m_n_buffer ; ++i)
		{
			if (m_buffers[i].start)
				free (m_buffers[i].start);
		}
		break;
	}
}

void CV4l2Cam::DeInit()
{
	if (m_fd)
	{
		close(m_fd);
		m_fd = 0;

		fprintf(stderr, "[%s] %s is closed\n", __func__, m_dev_name);

		free (m_dev_name);
		m_dev_name = NULL;
	}
}

void CV4l2Cam::Splitter_Input_Port_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	CV4l2Cam* pThis = (CV4l2Cam*)port->userdata;
	MMAL_BUFFER_HEADER_T* new_buffer;
    MMAL_STATUS_T status;

	if (buffer->cmd != 0)
   {
      LOG_INFO("%s callback: event %u not supported", port->name, buffer->cmd);
   }

   LOG_TRACE("%s\n", port->name);

   mmal_buffer_header_release(buffer);

   // and send one back to the port (if still open)
   if (port->is_enabled)
   {
      new_buffer = mmal_queue_get(pThis->m_splitter.input_pool->queue);

      if (new_buffer)
      {
    	  LOG_TRACE("new_buffer %p\n", new_buffer);
    	  status = mmal_port_send_buffer(port, new_buffer);
      }

      if (!new_buffer || status != MMAL_SUCCESS)
         fprintf(stderr, "Unable to return a buffer to the splitter port");
   }

//   LOG_TRACE("%s-%d\n", __func__, __LINE__);
}

void CV4l2Cam::Splitter_Outputput_Port_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   if (buffer->cmd != 0)
   {
      LOG_INFO("%s callback: event %u not supported", port->name, buffer->cmd);
   }

   mmal_buffer_header_release(buffer);
   LOG_TRACE("%s-%d\n", __func__, __LINE__);
}

MMAL_STATUS_T CV4l2Cam::CreateSplitterComponent(unsigned int buffer_size)
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	if (m_splitter.component)
		return status;

	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_SPLITTER, &m_splitter.component);

	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, "Failed to create video splitter component\n");
	}
	else
	{
		fprintf(stderr, "[%s] %s has been created with id %d, in %d, out %d\n", __func__, m_splitter.component->name,
				m_splitter.component->id, m_splitter.component->input_num, m_splitter.component->output_num);

		if (!m_splitter.component->input_num)
		{
		      status = MMAL_ENOSYS;
		      fprintf(stderr, "[%s] %s doesn't have any input port\n", __func__, m_splitter.component->name);
		      goto error;
		}

		if (m_splitter.component->output_num < 2)
		{
			status = MMAL_ENOSYS;
			fprintf(stderr, "[%s] %s doesn't have enough output ports", __func__, m_splitter.component->name);
			goto error;
		}

		if (m_splitter.component->input[0]->format)
		{
			m_splitter.component->input[0]->format->type	 = MMAL_ES_TYPE_VIDEO;
			m_splitter.component->input[0]->format->encoding = MMAL_ENCODING_I420;

			fprintf(stderr, "[%s] %s format: %d, %4.4s, %4.4s\n", __func__, m_splitter.component->name,
					(int)m_splitter.component->input[0]->format->type,
					(char*)&m_splitter.component->input[0]->format->encoding,
					(char*)&m_splitter.component->input[0]->format->encoding_variant);
		}

		if (m_splitter.component->input[0]->buffer_num < 3)
			m_splitter.component->input[0]->buffer_num = 3;

		status = mmal_port_format_commit(m_splitter.component->input[0]);

		if (status != MMAL_SUCCESS)
		{
			fprintf(stderr, "[%s] Unable to set format on %s input port\n", __func__, m_splitter.component->name);
			goto error;
		}

		status = mmal_component_enable(m_splitter.component);
		if (status != MMAL_SUCCESS)
		{
			fprintf(stderr, "[%s] %s couldn't enabled.\n", __func__, m_splitter.component->name);
			goto error;
		}

		m_splitter.input_port = m_splitter.component->input[0];
		if (m_splitter.input_port)
		{
			m_splitter.input_port->buffer_size = buffer_size;
			fprintf(stderr, "[%s] input port %s, %d, %d\n", __func__, m_splitter.input_port->name,
					m_splitter.input_port->buffer_num, m_splitter.input_port->buffer_size);

			m_splitter.input_pool = mmal_port_pool_create(m_splitter.input_port, m_splitter.input_port->buffer_num, m_splitter.input_port->buffer_size);

			if (!m_splitter.input_pool)
			{
				fprintf(stderr, "[%s] Failed to create buffer header pool for splitter input port %s", __func__, m_splitter.input_port->name);
				goto error;
			}

			m_splitter.input_port->userdata = (MMAL_PORT_USERDATA_T*)this;
			mmal_port_enable(m_splitter.input_port, Splitter_Input_Port_CB);
		}

		m_splitter.output_port = m_splitter.component->output[0];
		if (m_splitter.output_port)
		{
			mmal_format_copy(m_splitter.output_port->format, m_splitter.input_port->format);

			if (m_splitter.output_port->buffer_num < 3)
				m_splitter.output_port->buffer_num = 3;

			status = mmal_port_format_commit(m_splitter.output_port);

			if (status != MMAL_SUCCESS)
			{
				fprintf(stderr, "[%s] Unable to set format on %s output port\n", __func__, m_splitter.component->name);
				goto error;
			}

			m_splitter.output_port->buffer_size = buffer_size;
			fprintf(stderr, "[%s] output port %s, %d, %d\n", __func__, m_splitter.output_port->name,
					m_splitter.output_port->buffer_num, m_splitter.output_port->buffer_size);

			m_splitter.output_pool = mmal_port_pool_create(m_splitter.output_port, m_splitter.output_port->buffer_num, m_splitter.output_port->buffer_size);

			if (!m_splitter.output_pool)
			{
				fprintf(stderr, "[%s] Failed to create buffer header pool for splitter output port %s", __func__, m_splitter.output_port->name);
				goto error;
			}

			mmal_port_enable(m_splitter.output_port, Splitter_Outputput_Port_CB);
		}
	}

	return status;

error:

	DestroySplitterComponent();

	return status;
}

MMAL_STATUS_T CV4l2Cam::DestroySplitterComponent()
{
	MMAL_STATUS_T status = MMAL_SUCCESS;

	if (!m_splitter.component)
		 return status;

	if (m_splitter.input_pool)
	{
		mmal_port_pool_destroy(m_splitter.input_port, m_splitter.input_pool);
		m_splitter.input_pool = NULL;
	}

	if (m_splitter.output_pool)
	{
		mmal_port_pool_destroy(m_splitter.output_port, m_splitter.output_pool);
		m_splitter.output_pool = NULL;
	}

	status = mmal_component_destroy(m_splitter.component);

	m_splitter.input_port = NULL;
	m_splitter.component = NULL;

	return status;
}

void* CV4l2Cam::DoCapture(void* arg)
{
	CV4l2Cam* pThis = (CV4l2Cam*)arg;
	int ret = 0;
	struct pollfd fds[1];
	struct v4l2_buffer buf;
	uint32_t i;

	fds[0].fd 		= pThis->m_fd;
	fds[0].events 	= POLLIN;

	if (!pThis)
		return NULL;

	while (!pThis->m_terminate)
	{
		ret = poll(fds, 1, 2000);

		if (ret == 0)
		{
			fprintf(stderr, "[%s][%s] timeout!!!\n", pThis->m_dev_name, __func__);
		}
		else if (ret == -1)
		{
			fprintf(stderr, "[%s][%s] %d, %s!!!\n", pThis->m_dev_name, __func__, errno, strerror(errno));
		}
		else
		{
			switch (pThis->m_io_method)
			{
			case IO_METHOD_READ:
				break;
			case IO_METHOD_MMAP:
				buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory 	= V4L2_MEMORY_MMAP;

				if (-1 == pThis->XIoctl(pThis->m_fd, VIDIOC_DQBUF, &buf))
				{
					fprintf(stderr, "[%s][%s] VIDIOC_DQBUF failed(%d, %s)\n", pThis->m_dev_name, __func__, errno, strerror(errno));
					pThis->StopCapture();
					break;
				}

				if (buf.index >= pThis->m_n_buffer)
				{
					fprintf(stderr, "[%s][%s] Incorrect buffer index(%d/%d)\n", pThis->m_dev_name, __func__, buf.index, pThis->m_n_buffer);
					pThis->StopCapture();
					break;
				}

				// Process
				fprintf(stderr, "[%s][%s] processing [%d] %p, %u\n",
						pThis->m_dev_name, __func__,
						buf.index, pThis->m_buffers[buf.index].start, buf.bytesused);



				if (-1 == pThis->XIoctl(pThis->m_fd, VIDIOC_QBUF, &buf))
				{
					fprintf(stderr, "[%s][%s] VIDIOC_QBUF failed(%d, %s)\n", pThis->m_dev_name, __func__, errno, strerror(errno));
					pThis->StopCapture();
					break;
				}
				break;
			case IO_METHOD_USERPTR:
				memset(&buf, 0x0, sizeof(buf));

				buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory 	= V4L2_MEMORY_USERPTR;

				// Dequeue buffer
				if (-1 == pThis->XIoctl(pThis->m_fd, VIDIOC_DQBUF, &buf))
				{
					fprintf(stderr, "[%s][%s] VIDIOC_DQBUF failed(%d, %s)\n", pThis->m_dev_name, __func__, errno, strerror(errno));
					pThis->StopCapture();
					break;
				}

				for (i=0 ; i < pThis->m_n_buffer ; ++i)
				{
					if ((unsigned long)pThis->m_buffers[i].start == buf.m.userptr && pThis->m_buffers[i].length == buf.length)
						break;
				}

				if (buf.index >= pThis->m_n_buffer)
				{
					fprintf(stderr, "[%s][%s] Incorrect buffer index(%d/%d)\n", pThis->m_dev_name, __func__, buf.index, pThis->m_n_buffer);
					pThis->StopCapture();
					break;
				}

				// Process
				fprintf(stderr, "[%s][%s] processing [%d] %p, %u\n",
						pThis->m_dev_name, __func__,
						buf.index, pThis->m_buffers[buf.index].start, buf.bytesused);

				MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(pThis->m_splitter.input_pool->queue);
				if (buffer)
				{
					mmal_buffer_header_mem_lock(buffer);

					fprintf(stderr, "[%s][%s] buffer %p, %d\n",
							pThis->m_dev_name, __func__,
							buffer, buffer->alloc_size);

					buffer->offset = 0;
					memcpy(buffer->data, pThis->m_buffers[buf.index].start, buf.bytesused);
					mmal_buffer_header_mem_unlock(buffer);

                    if (mmal_port_send_buffer(pThis->m_splitter.input_port, buffer)!= MMAL_SUCCESS)
                       fprintf(stderr, "[%s][%s] Unable to send a buffer to splitter input port\n", pThis->m_dev_name, __func__);
				}

				// Queue buffer.
				if (-1 == pThis->XIoctl(pThis->m_fd, VIDIOC_QBUF, &buf))
				{
					fprintf(stderr, "[%s][%s] VIDIOC_QBUF failed(%d, %s)\n", pThis->m_dev_name, __func__, errno, strerror(errno));
					pThis->StopCapture();
					break;
				}
				break;
			}
		}
	}

	fprintf(stderr, "[%s][%s] Exit!!!\n", pThis->m_dev_name, __func__);

	return NULL;
}
// ---------------------------------------------------------------------------------
CV4l2Cam::CV4l2Cam()
{
	m_fd 		= 0;
	m_dev_name 	= NULL;

	m_io_method = IO_METHOD_USERPTR;
	m_n_buffer	= 0;
	m_buffers	= NULL;

	memset(&m_caps, 0x0, sizeof(m_caps));
	memset(&m_cropcap, 0x0, sizeof(m_cropcap));
	memset(&m_fmtdesc, 0x0, sizeof(m_fmtdesc));
	memset(&m_fmt, 0x0, sizeof(m_fmt));

	memset(&m_splitter, 0x0, sizeof(m_splitter));
}

CV4l2Cam::~CV4l2Cam()
{
	m_terminate = true;

	pthread_join(m_capture_thread, NULL);

	StopCapture();

	DestroySplitterComponent();

	DeInitMem();

	DeInit();
}

bool CV4l2Cam::Init(const char* dev_name)
{
	m_fd = open(dev_name, O_RDWR);
	if (m_fd == -1)
	{
		fprintf(stderr, "[%s] Can not open %s\n", __func__, dev_name);
		return false;
	}

	m_dev_name = strdup(dev_name);
	fprintf(stderr, "[%s] %s is opened\n", __func__, m_dev_name);

	QueryCap();

	Setup(1920, 1080);

	InitMem(m_fmt.fmt.pix.sizeimage);

	MMAL_STATUS_T status = CreateSplitterComponent(m_fmt.fmt.pix.sizeimage);

	m_cam_type = CamType_V4l2;

	StartCapture();

	return true;
}

bool CV4l2Cam::Setup(uint32_t width, uint32_t height)
{
    char fourcc[5] = {0};

	m_fmt.type 				= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	m_fmt.fmt.pix.width 	= width;
	m_fmt.fmt.pix.height 	= height;

	printf("Set Format:\n");
	if (-1 == XIoctl(m_fd, VIDIOC_S_FMT, &m_fmt))
	{
		perror("Querying VIDIOC_S_FMT");
		return false;
	}

    m_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ( -1 == XIoctl(m_fd, VIDIOC_G_FMT, &m_fmt))
	{
		perror("Querying VIDIOC_G_FMT");
		return false;
	}

    printf("Get Format:\n");
    struct v4l2_pix_format		pix;
    memcpy(&pix, &m_fmt.fmt.pix, sizeof(pix));

    strncpy(fourcc, (char *)&pix.pixelformat, 4);

    printf("%u %ux%u, pixelformat %s, field %u, byteperline %u, sizeimage %u, colorspace %u, flags 0x%X\n",
    		m_fmt.type,
			pix.width, pix.height,
			fourcc,
			pix.field,
			pix.bytesperline,
			pix.sizeimage,
			pix.colorspace,
			pix.flags);

	m_width 	= m_fmt.fmt.pix.width;
	m_height 	= m_fmt.fmt.pix.height;

	return true;
}

bool CV4l2Cam::StartCapture()
{
	bool ret = true;
	uint32_t i=0;
	enum v4l2_buf_type buf_type;

	switch (m_io_method)
	{
	case IO_METHOD_READ:
		break;
	case IO_METHOD_MMAP:
		for (i = 0 ; i < m_n_buffer ; ++i)
		{
			struct v4l2_buffer buf;

			memset(&buf, 0x0, sizeof(buf));
			buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory 	= V4L2_MEMORY_MMAP;
			buf.index	= i;

			if (-1 == XIoctl(m_fd, VIDIOC_QBUF, &buf))
			{
				ret = false;
				fprintf(stderr, "[%s][%s] VIDIOC_QBUF [%d] failed.\n", m_dev_name, __func__, i);
				break;
			}
		}

		if (ret == true)
		{
			buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == XIoctl(m_fd, VIDIOC_STREAMON, &buf_type))
			{
				ret = false;
				fprintf(stderr, "[%s][%s] VIDIOC_STREAMON failed.\n", m_dev_name, __func__);
			}
		}

		m_do_capture = true;
		pthread_create(&m_capture_thread, NULL, DoCapture, this);
		break;
	case IO_METHOD_USERPTR:
		for (i = 0 ; i < m_n_buffer ; ++i)
		{
			struct v4l2_buffer buf;

			memset(&buf, 0x0, sizeof(buf));
			buf.type 		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory 		= V4L2_MEMORY_USERPTR;
			buf.index		= i;
			buf.m.userptr 	= (unsigned long)m_buffers[i].start;
			buf.length  	= m_buffers[i].length;

			if (-1 == XIoctl(m_fd, VIDIOC_QBUF, &buf))
			{
				ret = false;
				fprintf(stderr, "[%s][%s] VIDIOC_QBUF [%d] failed.\n", m_dev_name, __func__, i);
				break;
			}
		}

		if (ret == true)
		{
			buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == XIoctl(m_fd, VIDIOC_STREAMON, &buf_type))
			{
				ret = false;
				fprintf(stderr, "[%s][%s] VIDIOC_STREAMON failed.\n", m_dev_name, __func__);
			}
		}

		m_do_capture = true;
		pthread_create(&m_capture_thread, NULL, DoCapture, this);
		break;
	}

	return ret;
}

bool CV4l2Cam::StopCapture()
{
	bool ret = true;
	enum v4l2_buf_type buf_type;

	if (!m_do_capture)
		return true;

	m_do_capture = false;

	switch (m_io_method)
	{
	case IO_METHOD_READ:
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == XIoctl(m_fd, VIDIOC_STREAMOFF, &buf_type))
		{
			ret = false;
			fprintf(stderr, "[%s][%s] VIDIOC_STREAMOFF failed.\n", m_dev_name, __func__);
		}
		break;
	}

	return true;
}
