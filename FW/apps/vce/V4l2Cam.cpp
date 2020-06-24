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

	return ret;
}

bool CV4l2Cam::InitMemRead()
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

bool CV4l2Cam::InitMemUserp()
{
	bool  ret = true;

	return ret;
}

bool CV4l2Cam::InitMem()
{
	bool  ret = true;

	switch (m_io_method)
	{
	case IO_METHOD_READ:
		ret = InitMemRead();
		break;
	case IO_METHOD_MMAP:
		ret = InitMemMmap();
		break;
	case IO_METHOD_USERPTR:
		ret = InitMemUserp();
		break;
	}

	return ret;
}

void CV4l2Cam::DeInitMem()
{
	switch (m_io_method)
	{
	case IO_METHOD_READ:
		if (m_buffers[0].start)
			free (m_buffers[0].start);
		break;
	case IO_METHOD_MMAP:
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
				fprintf(stderr, "[%s][%s] processing [%d] %p\n", pThis->m_dev_name, __func__, buf.index, pThis->m_buffers[buf.index].start);



				if (-1 == pThis->XIoctl(pThis->m_fd, VIDIOC_QBUF, &buf))
				{
					fprintf(stderr, "[%s][%s] VIDIOC_QBUF failed(%d, %s)\n", pThis->m_dev_name, __func__, errno, strerror(errno));
					pThis->StopCapture();
					break;
				}
				break;
			case IO_METHOD_USERPTR:

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

	m_io_method = IO_METHOD_MMAP;
	m_n_buffer	= 0;
	m_buffers	= NULL;

	memset(&m_caps, 0x0, sizeof(m_caps));
	memset(&m_cropcap, 0x0, sizeof(m_cropcap));
	memset(&m_fmtdesc, 0x0, sizeof(m_fmtdesc));
	memset(&m_fmt, 0x0, sizeof(m_fmt));
}

CV4l2Cam::~CV4l2Cam()
{
	m_terminate = true;

	pthread_join(m_capture_thread, NULL);

	StopCapture();

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

	Setup(800, 600);

	InitMem();

	m_cam_type = CamType_V4l2;

	StartCapture();

	return true;
}

bool CV4l2Cam::Setup(uint32_t width, uint32_t height)
{
	m_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	m_fmt.fmt.pix.width = width;
	m_fmt.fmt.pix.height = height;

	if (-1 == XIoctl(m_fd, VIDIOC_S_FMT, &m_fmt))
	{
		perror("Querying VIDIOC_S_FMT");
		return false;
	}

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

	m_do_capture = false;

	return true;
}
