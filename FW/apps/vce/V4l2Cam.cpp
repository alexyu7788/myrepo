#include "V4l2Cam.h"

// ---------------------------------------------------------------------------------
#define V4L2_BUFFER_DEFAULT		8
#define V4L2_BUFFER_MAX			32

#define MMAL_ENCODING_UNUSED 	0

static struct v4l2_field_name_s v4l2_field_name_tbl[] = {
	{ "any", V4L2_FIELD_ANY },
	{ "none", V4L2_FIELD_NONE },
	{ "top", V4L2_FIELD_TOP },
	{ "bottom", V4L2_FIELD_BOTTOM },
	{ "interlaced", V4L2_FIELD_INTERLACED },
	{ "seq-tb", V4L2_FIELD_SEQ_TB },
	{ "seq-bt", V4L2_FIELD_SEQ_BT },
	{ "alternate", V4L2_FIELD_ALTERNATE },
	{ "interlaced-tb", V4L2_FIELD_INTERLACED_TB },
	{ "interlaced-bt", V4L2_FIELD_INTERLACED_BT },
};

static struct v4l2_mmal_format_info v4l2_mmal_formats_tbl[] = {
	{ "RGB332", V4L2_PIX_FMT_RGB332, 1, 	MMAL_ENCODING_UNUSED },
	{ "RGB444", V4L2_PIX_FMT_RGB444, 1,	MMAL_ENCODING_UNUSED },
	{ "ARGB444", V4L2_PIX_FMT_ARGB444, 1,	MMAL_ENCODING_UNUSED },
	{ "XRGB444", V4L2_PIX_FMT_XRGB444, 1,	MMAL_ENCODING_UNUSED },
	{ "RGB555", V4L2_PIX_FMT_RGB555, 1,	MMAL_ENCODING_UNUSED },
	{ "ARGB555", V4L2_PIX_FMT_ARGB555, 1,	MMAL_ENCODING_UNUSED },
	{ "XRGB555", V4L2_PIX_FMT_XRGB555, 1,	MMAL_ENCODING_UNUSED },
	{ "RGB565", V4L2_PIX_FMT_RGB565, 1,	MMAL_ENCODING_UNUSED },
	{ "RGB555X", V4L2_PIX_FMT_RGB555X, 1,	MMAL_ENCODING_UNUSED },
	{ "RGB565X", V4L2_PIX_FMT_RGB565X, 1,	MMAL_ENCODING_RGB16 },
	{ "BGR666", V4L2_PIX_FMT_BGR666, 1,	MMAL_ENCODING_UNUSED },
	{ "BGR24", V4L2_PIX_FMT_BGR24, 1,	MMAL_ENCODING_RGB24 },
	{ "RGB24", V4L2_PIX_FMT_RGB24, 1,	MMAL_ENCODING_BGR24 },
	{ "BGR32", V4L2_PIX_FMT_BGR32, 1,	MMAL_ENCODING_BGR32 },
	{ "ABGR32", V4L2_PIX_FMT_ABGR32, 1,	MMAL_ENCODING_BGRA },
	{ "XBGR32", V4L2_PIX_FMT_XBGR32, 1,	MMAL_ENCODING_BGR32 },
	{ "RGB32", V4L2_PIX_FMT_RGB32, 1,	MMAL_ENCODING_RGB32 },
	{ "ARGB32", V4L2_PIX_FMT_ARGB32, 1,	MMAL_ENCODING_ARGB },
	{ "XRGB32", V4L2_PIX_FMT_XRGB32, 1,	MMAL_ENCODING_UNUSED },
	{ "HSV24", V4L2_PIX_FMT_HSV24, 1,	MMAL_ENCODING_UNUSED },
	{ "HSV32", V4L2_PIX_FMT_HSV32, 1,	MMAL_ENCODING_UNUSED },
	{ "Y8", V4L2_PIX_FMT_GREY, 1,		MMAL_ENCODING_UNUSED },
	{ "Y10", V4L2_PIX_FMT_Y10, 1,		MMAL_ENCODING_UNUSED },
	{ "Y12", V4L2_PIX_FMT_Y12, 1,		MMAL_ENCODING_UNUSED },
	{ "Y16", V4L2_PIX_FMT_Y16, 1,		MMAL_ENCODING_UNUSED },
	{ "UYVY", V4L2_PIX_FMT_UYVY, 1,		MMAL_ENCODING_UYVY },
	{ "VYUY", V4L2_PIX_FMT_VYUY, 1,		MMAL_ENCODING_VYUY },
	{ "YUYV", V4L2_PIX_FMT_YUYV, 1,		MMAL_ENCODING_YUYV },
	{ "YVYU", V4L2_PIX_FMT_YVYU, 1,		MMAL_ENCODING_YVYU },
	{ "NV12", V4L2_PIX_FMT_NV12, 1,		MMAL_ENCODING_NV12 },
	{ "NV12M", V4L2_PIX_FMT_NV12M, 2,	MMAL_ENCODING_UNUSED },
	{ "NV21", V4L2_PIX_FMT_NV21, 1,		MMAL_ENCODING_NV21 },
	{ "NV21M", V4L2_PIX_FMT_NV21M, 2,	MMAL_ENCODING_UNUSED },
	{ "NV16", V4L2_PIX_FMT_NV16, 1,		MMAL_ENCODING_UNUSED },
	{ "NV16M", V4L2_PIX_FMT_NV16M, 2,	MMAL_ENCODING_UNUSED },
	{ "NV61", V4L2_PIX_FMT_NV61, 1,		MMAL_ENCODING_UNUSED },
	{ "NV61M", V4L2_PIX_FMT_NV61M, 2,	MMAL_ENCODING_UNUSED },
	{ "NV24", V4L2_PIX_FMT_NV24, 1,		MMAL_ENCODING_UNUSED },
	{ "NV42", V4L2_PIX_FMT_NV42, 1,		MMAL_ENCODING_UNUSED },
	{ "YUV420M", V4L2_PIX_FMT_YUV420M, 3,	MMAL_ENCODING_UNUSED },
	{ "YUV422M", V4L2_PIX_FMT_YUV422M, 3,	MMAL_ENCODING_UNUSED },
	{ "YUV444M", V4L2_PIX_FMT_YUV444M, 3,	MMAL_ENCODING_UNUSED },
	{ "YVU420M", V4L2_PIX_FMT_YVU420M, 3,	MMAL_ENCODING_UNUSED },
	{ "YVU422M", V4L2_PIX_FMT_YVU422M, 3,	MMAL_ENCODING_UNUSED },
	{ "YVU444M", V4L2_PIX_FMT_YVU444M, 3,	MMAL_ENCODING_UNUSED },
	{ "SBGGR8", V4L2_PIX_FMT_SBGGR8, 1,	MMAL_ENCODING_BAYER_SBGGR8 },
	{ "SGBRG8", V4L2_PIX_FMT_SGBRG8, 1,	MMAL_ENCODING_BAYER_SGBRG8 },
	{ "SGRBG8", V4L2_PIX_FMT_SGRBG8, 1,	MMAL_ENCODING_BAYER_SGRBG8 },
	{ "SRGGB8", V4L2_PIX_FMT_SRGGB8, 1,	MMAL_ENCODING_BAYER_SRGGB8 },
	{ "SBGGR10_DPCM8", V4L2_PIX_FMT_SBGGR10DPCM8, 1,	MMAL_ENCODING_UNUSED },
	{ "SGBRG10_DPCM8", V4L2_PIX_FMT_SGBRG10DPCM8, 1,	MMAL_ENCODING_UNUSED },
	{ "SGRBG10_DPCM8", V4L2_PIX_FMT_SGRBG10DPCM8, 1,	MMAL_ENCODING_UNUSED },
	{ "SRGGB10_DPCM8", V4L2_PIX_FMT_SRGGB10DPCM8, 1,	MMAL_ENCODING_UNUSED },
	{ "SBGGR10", V4L2_PIX_FMT_SBGGR10, 1,	MMAL_ENCODING_UNUSED },
	{ "SGBRG10", V4L2_PIX_FMT_SGBRG10, 1,	MMAL_ENCODING_UNUSED },
	{ "SGRBG10", V4L2_PIX_FMT_SGRBG10, 1,	MMAL_ENCODING_UNUSED },
	{ "SRGGB10", V4L2_PIX_FMT_SRGGB10, 1,	MMAL_ENCODING_UNUSED },
	{ "SBGGR10P", V4L2_PIX_FMT_SBGGR10P, 1,	MMAL_ENCODING_BAYER_SBGGR10P },
	{ "SGBRG10P", V4L2_PIX_FMT_SGBRG10P, 1,	MMAL_ENCODING_BAYER_SGBRG10P },
	{ "SGRBG10P", V4L2_PIX_FMT_SGRBG10P, 1,	MMAL_ENCODING_BAYER_SGRBG10P },
	{ "SRGGB10P", V4L2_PIX_FMT_SRGGB10P, 1,	MMAL_ENCODING_BAYER_SRGGB10P },
	{ "SBGGR12", V4L2_PIX_FMT_SBGGR12, 1,	MMAL_ENCODING_UNUSED },
	{ "SGBRG12", V4L2_PIX_FMT_SGBRG12, 1,	MMAL_ENCODING_UNUSED },
	{ "SGRBG12", V4L2_PIX_FMT_SGRBG12, 1,	MMAL_ENCODING_UNUSED },
	{ "SRGGB12", V4L2_PIX_FMT_SRGGB12, 1,	MMAL_ENCODING_UNUSED },
	{ "DV", V4L2_PIX_FMT_DV, 1,		MMAL_ENCODING_UNUSED },
	{ "MJPEG", V4L2_PIX_FMT_MJPEG, 1,	MMAL_ENCODING_UNUSED },
	{ "MPEG", V4L2_PIX_FMT_MPEG, 1,		MMAL_ENCODING_UNUSED },
};
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

	fprintf(stderr, "==================%s==================\n", __func__);
	fprintf(stderr, "Driver Caps:\n"
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
	fprintf(stderr, "Cropping Capability:\n");

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

		printf("\t[%u] %s: %c%c %s\n", m_fmtdesc.index++, fourcc, c, e, m_fmtdesc.description);
    }

    fprintf(stderr, "===========================================\n\n");

	return ret;
}

int CV4l2Cam::QueryFps()
{
	struct v4l2_streamparm parm;
	int ret;

	memset(&parm, 0, sizeof parm);
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(m_fd, VIDIOC_G_PARM, &parm);
	if (ret < 0) {
		printf("Unable to get frame rate: %s (%d).\n",
			strerror(errno), errno);
		/* Make a wild guess at the frame rate */
		m_fps = 15;
		return ret;
	}

	printf("Current frame rate: %u/%u\n",
		parm.parm.capture.timeperframe.denominator,
		parm.parm.capture.timeperframe.numerator);

	m_fps = parm.parm.capture.timeperframe.denominator/
			parm.parm.capture.timeperframe.numerator;

	return 0;
}

int CV4l2Cam::VideoSetFormat(unsigned int w, unsigned int h,
							unsigned int format, unsigned int stride,
							unsigned int buffer_size, __u32 field,
							unsigned int flags)
{
	struct v4l2_format fmt;
	int ret;

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	fmt.fmt.pix.width = w;
	fmt.fmt.pix.height = h;
	fmt.fmt.pix.pixelformat = format;
	fmt.fmt.pix.field = field;
	printf("stride is %d\n",stride);
	if (!stride)
		stride = ((w+31) &~31)*format_bpp(format);
	printf("stride is now %d\n",stride);
	fmt.fmt.pix.bytesperline = stride;
	fmt.fmt.pix.sizeimage = buffer_size;
	fmt.fmt.pix.priv = V4L2_PIX_FMT_PRIV_MAGIC;
	fmt.fmt.pix.flags = flags;

	ret = ioctl(m_fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		printf("Unable to set format: %s (%d).\n", strerror(errno),
			errno);
		return ret;
	}

	printf("Video format set: %s (%08x) %ux%u (stride %u) field %s buffer size %u\n",
		v4l2_format_name(fmt.fmt.pix.pixelformat), fmt.fmt.pix.pixelformat,
		fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.bytesperline,
		v4l2_field_name(fmt.fmt.pix.field),
		fmt.fmt.pix.sizeimage);

	return 0;
}

int CV4l2Cam::VideoGetFormat()
{
	struct v4l2_format fmt;
	int ret;

	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(m_fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		printf("Unable to get format: %s (%d).\n", strerror(errno),
			errno);
		return ret;
	}

	m_width 		= fmt.fmt.pix.width;
	m_height 		= fmt.fmt.pix.height;
	m_num_planes 	= 1;

	printf("Video format: \n\t%s (%08x) %ux%u (stride %u) field %s buffer size %u\n",
								v4l2_format_name(fmt.fmt.pix.pixelformat),
								fmt.fmt.pix.pixelformat,
								fmt.fmt.pix.width,
								fmt.fmt.pix.height,
								fmt.fmt.pix.bytesperline,
								v4l2_field_name(fmt.fmt.pix_mp.field),
								fmt.fmt.pix.sizeimage);

	return 0;
}

void CV4l2Cam::get_ts_flags(uint32_t flags, const char **ts_type, const char **ts_source)
{
	switch (flags & V4L2_BUF_FLAG_TIMESTAMP_MASK)
	{
		case V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN:
			*ts_type = "unk";
			break;
		case V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC:
			*ts_type = "mono";
			break;
		case V4L2_BUF_FLAG_TIMESTAMP_COPY:
			*ts_type = "copy";
			break;
		default:
			*ts_type = "inv";
			break;
	}

	switch (flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK)
	{
		case V4L2_BUF_FLAG_TSTAMP_SRC_EOF:
			*ts_source = "EoF";
			break;
		case V4L2_BUF_FLAG_TSTAMP_SRC_SOE:
			*ts_source = "SoE";
			break;
		default:
			*ts_source = "inv";
			break;
	}
}

bool CV4l2Cam::InitMemRead(unsigned int buffer_size)
{
	bool  ret = true;

	return ret;
}

bool CV4l2Cam::InitMemMmap()
{
	bool  ret = true;
	struct v4l2_buffer	buf;
	struct v4l2_requestbuffers req;
	struct v4l2_plane planes[VIDEO_MAX_PLANES];

	memset(&req, 0x0, sizeof(req));

	req.count 	= V4L2_BUFFER_DEFAULT;
	req.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory 	= V4L2_MEMORY_MMAP;

	if (-1 == XIoctl(m_fd, VIDIOC_REQBUFS, &req))
	{
		switch (errno)
		{
		case EINVAL:
			fprintf(stderr, PRINTF_COLOR_RED "[%s] does not support memory mapping.\n" PRINTF_COLOR_NONE, __func__);
			ret = false;
			break;
		default:
			fprintf(stderr, PRINTF_COLOR_RED "[%s] VIDIOC_REQBUFS: %d, %s\n" PRINTF_COLOR_NONE, __func__, errno, strerror(errno));
			ret = false;
			break;
		}
	}

	if (req.count < 2)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] Insufficient buffer memory.\n" PRINTF_COLOR_NONE, __func__);
		ret = false;
	}

	if (ret == false)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] VIDIOC_REQBUFS failed.\n" PRINTF_COLOR_NONE, __func__);
		return ret;
	}

	fprintf(stderr, PRINTF_COLOR_GREEN "[%s] %u buffers requested.\n" PRINTF_COLOR_NONE, __func__, req.count);

	m_buffers = (struct buffer*)malloc(req.count * sizeof(struct buffer));
	if (!m_buffers)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s]: Out of memory.\n" PRINTF_COLOR_NONE, __func__);
		return false;
	}

	for (m_n_buffer = 0 ; m_n_buffer < req.count ; ++m_n_buffer)
	{
		const char *ts_type, *ts_source;

		memset(&buf, 0x0, sizeof(buf));
		memset(planes, 0x0, sizeof(planes));

		buf.index	= m_n_buffer;
		buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory 	= V4L2_MEMORY_MMAP;
		buf.length  = VIDEO_MAX_PLANES;
		buf.m.planes = planes;

		if (-1 == XIoctl(m_fd, VIDIOC_QUERYBUF, &buf))
		{
			fprintf(stderr, PRINTF_COLOR_RED "[%s] VIDIOC_QUERYBUF failed.\n" PRINTF_COLOR_NONE, __func__);
			ret = false;
			break;
		}

		get_ts_flags(buf.flags, &ts_type, &ts_source);
		fprintf(stderr, PRINTF_COLOR_GREEN "[%s] buf[%d] length: %u offset: %u timestamp type/source: %s/%s\n" PRINTF_COLOR_NONE, __func__, m_n_buffer,
		       buf.length, buf.m.offset, ts_type, ts_source);

		m_buffers[m_n_buffer].idx = m_n_buffer;

		// Memory Mapping...
		for (unsigned char i=0 ; i<m_num_planes ; ++i)
		{
			m_buffers[m_n_buffer].start[i] = (struct buffer*)mmap(0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, buf.m.offset);

			if (MAP_FAILED == m_buffers[m_n_buffer].start[i])
			{
				fprintf(stderr, PRINTF_COLOR_RED "[%s] MMAP failed.\n" PRINTF_COLOR_NONE, __func__);
				ret = false;
				break;
			}

			if (ret == false)
				break;

			m_buffers[m_n_buffer].length[i] = buf.length;

			fprintf(stderr, PRINTF_COLOR_GREEN "[%s] Buffer %u/%u mapped at address %p.\n" PRINTF_COLOR_NONE, __func__, m_buffers[m_n_buffer].idx, i, m_buffers[m_n_buffer].start[i]);
		}

		if (ret == false)
			return ret;

		// Exporting mmal buffer as dmafd
		if (m_video_source.input.pool)
		{
			struct v4l2_exportbuffer expbuf;
			MMAL_BUFFER_HEADER_T *bh;

			bh = mmal_queue_get(m_video_source.input.pool->queue);
			if (!bh)
			{
				fprintf(stderr, PRINTF_COLOR_RED  "[%s] Failed to get a buffer from the pool. Queue length %d\n" PRINTF_COLOR_NONE, __func__, mmal_queue_length(m_video_source.input.pool->queue));
				return false;
			}

			bh->user_data = &m_buffers[m_n_buffer];

			memset(&expbuf, 0, sizeof(expbuf));
			expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			expbuf.index = m_n_buffer;

			if (!ioctl(m_fd, VIDIOC_EXPBUF, &expbuf))
			{
				m_buffers[m_n_buffer].dma_fd = expbuf.fd;
				m_buffers[m_n_buffer].vcsm_handle = vcsm_import_dmabuf(expbuf.fd, "V4L2 buf");
			}
			else
			{
				m_buffers[m_n_buffer].dma_fd = -1;
				m_buffers[m_n_buffer].vcsm_handle = 0;
			}

			if (m_buffers[m_n_buffer].vcsm_handle > 0)
			{
				m_zero_copy = MMAL_TRUE;
				fprintf(stderr, PRINTF_COLOR_GREEN  "[%s] Exported buffer %d to dmabuf %d, vcsm handle %u\n" PRINTF_COLOR_NONE, __func__, m_n_buffer, m_buffers[m_n_buffer].dma_fd, m_buffers[m_n_buffer].vcsm_handle);
				bh->data = (uint8_t*)vcsm_vc_hdl_from_hdl(m_buffers[m_n_buffer].vcsm_handle);
			}
			else
			{
				m_zero_copy = MMAL_FALSE;
				bh->data = (uint8_t*)m_buffers[m_n_buffer].start[0];	//Only deal with the single planar API
			}

			bh->alloc_size = buf.length;
			m_buffers[m_n_buffer].bufferheader = bh;
			fprintf(stderr, PRINTF_COLOR_GREEN "[%s] Linking V4L2 buffer[%d] %p to buffer header %p. bh->data 0x%x, dmafd %d\n"  PRINTF_COLOR_NONE, __func__,
					m_n_buffer, &m_buffers[m_n_buffer], bh, (uint32_t)bh->data, m_buffers[m_n_buffer].dma_fd);

			/* Put buffer back in the pool */
			mmal_buffer_header_release(bh);
		}
	}

	m_timestamp_type = buf.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK;

	return ret;
}

bool CV4l2Cam::InitMemUserPtr(unsigned int buffer_size)
{
	bool  ret = true;
//	struct v4l2_requestbuffers req;
//
//	memset(&req, 0x0, sizeof(req));
//
//	req.count 	= 4;
//	req.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
//	req.memory 	= V4L2_MEMORY_USERPTR;
//
//	if (-1 == XIoctl(m_fd, VIDIOC_REQBUFS, &req))
//	{
//		switch (errno)
//		{
//		case EINVAL:
//			fprintf(stderr, "[%s] does not support memory mapping.\n", m_dev_name);
//			ret = false;
//			break;
//		default:
//			fprintf(stderr, "[%s] VIDIOC_REQBUFS: %d, %s\n", m_dev_name, errno, strerror(errno));
//			ret = false;
//			break;
//		}
//	}
//
//	m_buffers = (struct buffer*)calloc(req.count, sizeof(*m_buffers));
//	if (!m_buffers)
//	{
//		fprintf(stderr, "[%s][%s]: Out of memory.\n", m_dev_name, __func__);
//		return false;
//	}
//
//	for (m_n_buffer = 0 ; m_n_buffer < req.count ; ++m_n_buffer)
//	{
//		m_buffers[m_n_buffer].length = buffer_size;
//		m_buffers[m_n_buffer].start  = malloc(buffer_size);
//
//		if (!m_buffers[m_n_buffer].start)
//		{
//			fprintf(stderr, "[%s][%s] UserPtr failed.\n", m_dev_name, __func__);
//			ret = false;
//			break;
//		}
//
//		fprintf(stderr, "[%s][%s] UserPtr [%d]: %p, %d.\n", m_dev_name, __func__, m_n_buffer, m_buffers[m_n_buffer].start, m_buffers[m_n_buffer].length);
//	}

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
	int ret;
	struct v4l2_requestbuffers rb;

	switch (m_io_method)
	{
	case IO_METHOD_READ:
		fprintf(stderr, "[%s] IO_METHOD_READ.\n", __func__);
		if (m_buffers[0].start)
			free (m_buffers[0].start);
		break;
	case IO_METHOD_MMAP:
		fprintf(stderr, "[%s] IO_METHOD_MMAP.\n", __func__);
		for (uint32_t i=0 ; i < m_n_buffer ; ++i)
		{
			if (m_buffers[i].vcsm_handle)
			{
				fprintf(stderr, "[%s] Releasing vcsm handle %u\n", __func__, m_buffers[i].vcsm_handle);
				vcsm_free(m_buffers[i].vcsm_handle);
			}

			if (m_buffers[i].dma_fd >= 0)
			{
				fprintf(stderr, "[%s] Closing dma_buf %d\n", __func__, m_buffers[i].dma_fd);
				close(m_buffers[i].dma_fd);
			}

			for (unsigned char j=0 ; j<m_num_planes ; ++j)
			{
				if (m_buffers[i].start[j])
				{
					if (-1 == munmap(m_buffers[i].start[j], m_buffers[i].length[j]))
					{
						fprintf(stderr, "[%s] munmap failed.(%d, %s)\n", __func__, errno, strerror(errno));
					}
				}
			}
		}

		memset(&rb, 0, sizeof rb);
		rb.count = 0;
		rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		rb.memory = V4L2_MEMORY_MMAP;

		ret = ioctl(m_fd, VIDIOC_REQBUFS, &rb);
		if (ret < 0) {
			fprintf(stderr, "[%s] Unable to release buffers: %s (%d).\n", __func__,
				strerror(errno), errno);
			return;
		}
		break;
	case IO_METHOD_USERPTR:
		fprintf(stderr, "[%s] IO_METHOD_USERPTR.\n", __func__);
		for (uint32_t i=0 ; i < m_n_buffer ; ++i)
		{
			if (m_buffers[i].start)
				free (m_buffers[i].start);
		}
		break;
	}

	if (m_buffers)
		free (m_buffers);

	m_n_buffer = 0;
	m_buffers = NULL;
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


int CV4l2Cam::QueueBuffer(unsigned int idx)
{
	int ret = 0;
	struct v4l2_buffer buf;

	switch (m_io_method)
	{
	case IO_METHOD_MMAP:
		memset(&buf, 0x0, sizeof(buf));
		buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory 	= V4L2_MEMORY_MMAP;
		buf.index	= idx;

		if (-1 == XIoctl(m_fd, VIDIOC_QBUF, &buf))
		{
			ret = -1;
			fprintf(stderr, "[%s][%s] VIDIOC_QBUF [%d] failed.\n", m_dev_name, __func__, idx);
			break;
		}
		break;
	case IO_METHOD_USERPTR:
//		memset(&buf, 0x0, sizeof(buf));
//		buf.type 		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
//		buf.memory 		= V4L2_MEMORY_USERPTR;
//		buf.index		= idx;
//		buf.m.userptr 	= (unsigned long)m_buffers[idx].start;
//		buf.length  	= m_buffers[idx].length;
//
//		if (-1 == XIoctl(m_fd, VIDIOC_QBUF, &buf))
//		{
//			ret = -1;
//			fprintf(stderr, "[%s][%s] VIDIOC_QBUF [%d] failed.\n", m_dev_name, __func__, idx);
//			break;
//		}
		break;
	}

	return ret;
}

int CV4l2Cam::QueueAllBuffer()
{
	int ret = 0;

	for (uint32_t i = 0 ; i < m_n_buffer ; ++i)
	{
		ret = QueueBuffer(i);
		if (ret == -1)
			break;
	}

	return ret;
}

void CV4l2Cam::ReturnBuffersToPort(struct port_info* port_info)
{
	MMAL_BUFFER_HEADER_T *buffer;

	if (!port_info || !port_info->port || !port_info->pool)
		return;

	while ((buffer = mmal_queue_get(port_info->pool->queue)) != NULL)
	{
		mmal_port_send_buffer(port_info->port, buffer);
	}
}

void CV4l2Cam::Splitter_Input_Port_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	CV4l2Cam* pThis = (CV4l2Cam*)port->userdata;
	MMAL_BUFFER_HEADER_T* new_buffer;
    MMAL_STATUS_T status;

	if (buffer->cmd != 0)
	{
		fprintf(stderr, PRINTF_COLOR_RED "%s callback: event %u not supported.\n" PRINTF_COLOR_NONE, port->name, buffer->cmd);
	}

	fprintf(stderr, PRINTF_COLOR_LIGHT_BLUE "[%s] %s\n" PRINTF_COLOR_NONE, __func__, port->name);

	for (unsigned int i=0 ; i<pThis->m_n_buffer ; ++i)
	{
	   if (pThis->m_buffers[i].bufferheader == buffer)
	   {
		   pThis->QueueBuffer(pThis->m_buffers[i].idx);
		   mmal_buffer_header_release(buffer);
		   buffer = NULL;
		   break;
	   }
	}

	if (buffer)
	{
	   fprintf(stderr, PRINTF_COLOR_RED "Failed to find matching V4L2 buffer for mmal buffer %p\n" PRINTF_COLOR_NONE, buffer);
	   mmal_buffer_header_release(buffer);
	}
}

void CV4l2Cam::Splitter_Outputput_Port_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	struct timespec spec;
	static int64_t starttime = -1, curtime;
	struct port_info* pThis = (struct port_info*)port->userdata;

	clock_gettime(CLOCK_MONOTONIC_RAW, &spec);

	curtime = (spec.tv_sec * 1000000ULL) + (spec.tv_nsec / 1000);

	if (starttime == -1)
	{
		starttime = curtime;
		pThis->frames = 0;
	}
	else
	{
		if ((curtime - starttime) >= 1000000)
		{
			fprintf(stderr, PRINTF_COLOR_GREEN "[%s] %s, frame %u\n" PRINTF_COLOR_NONE, __func__, port->name, pThis->frames);
			starttime = -1;
			pThis->frames = 0;
		}
	}

	++pThis->frames;

	if (buffer->cmd != 0)
	{
		fprintf(stderr, PRINTF_COLOR_RED "%s callback: event %u not supported.\n" PRINTF_COLOR_NONE, port->name, buffer->cmd);
	}

	if (buffer->length)
		fprintf(stderr, PRINTF_COLOR_BLUE "[%s] buffer header's pts %lld ms of port[%d]\n" PRINTF_COLOR_NONE, __func__, buffer->pts, pThis->idx);

	// Transmit buffer to port pool of sink component.
	if (pThis->conn_comp && pThis->sink_pool)
	{
//		fprintf(stderr, PRINTF_COLOR_BLUE "[%s] conn_comp %s\n" PRINTF_COLOR_NONE, __func__, pThis->conn_comp->name);

		MMAL_BUFFER_HEADER_T *out = mmal_queue_get(pThis->sink_pool->queue);
		if (out)
		{
			mmal_buffer_header_replicate(out, buffer);
			mmal_port_send_buffer(pThis->conn_comp->input[0], out);
		}
	}

	mmal_buffer_header_release(buffer);

	if (pThis->return_buf_to_port)
		pThis->return_buf_to_port(pThis);

//	fprintf(stderr, PRINTF_COLOR_BLUE "[%s] %s\n" PRINTF_COLOR_NONE, __func__, port->name);
}

const v4l2_mmal_format_info* CV4l2Cam::V4l2FormatToMmalByFourcc(unsigned int forcc)
{
	unsigned int i;

	for (i=0 ; i < ARRAY_SIZE(v4l2_mmal_formats_tbl) ; ++i)
	{
		if (v4l2_mmal_formats_tbl[i].fourcc == forcc)
			return &v4l2_mmal_formats_tbl[i];
	}

	printf("%s: Unknown format %s\n", __func__, (char*)&forcc);

	return NULL;
}

int CV4l2Cam::format_bpp(unsigned int  pixfmt)
{
	switch(pixfmt)
	{
		case V4L2_PIX_FMT_BGR24:
		case V4L2_PIX_FMT_RGB24:
			return 4;
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_YVYU:
		case V4L2_PIX_FMT_UYVY:
		case V4L2_PIX_FMT_VYUY:
			return 2;
		case V4L2_PIX_FMT_SRGGB8:
		case V4L2_PIX_FMT_SBGGR8:
		case V4L2_PIX_FMT_SGRBG8:
		case V4L2_PIX_FMT_SGBRG8:
			return 1;
		default:
			return 1;
	}
}

const char *CV4l2Cam::v4l2_field_name(__u32 field)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(v4l2_field_name_tbl); ++i) {
		if (v4l2_field_name_tbl[i].field == field)
			return v4l2_field_name_tbl[i].name;
	}

	return "unknown";
}

const char *CV4l2Cam::v4l2_format_name(unsigned int fourcc)
{
	const struct v4l2_mmal_format_info *info;
	static char name[5];
	unsigned int i;

	info = V4l2FormatToMmalByFourcc(fourcc);
	if (info)
		return info->name;

	for (i = 0; i < 4; ++i) {
		name[i] = fourcc & 0xff;
		fourcc >>= 8;
	}

	name[4] = '\0';
	return name;
}

MMAL_STATUS_T CV4l2Cam::CreateSplitterComponent()
{
	MMAL_ES_FORMAT_T *format = NULL;
	const v4l2_mmal_format_info* info = NULL;
	MMAL_STATUS_T status = MMAL_SUCCESS;

	info = V4l2FormatToMmalByFourcc(m_fmt.fmt.pix.pixelformat);

	if (!info || info->mmal_encoding == MMAL_ENCODING_UNUSED)
	{
		fprintf(stderr, PRINTF_COLOR_RED "[%s] %s Unsupported encoding\n" PRINTF_COLOR_NONE, __func__, m_dev_name);
		goto error;
	}

	status = CreateComponent(&m_video_source, "vc.ril.isp");
//	status = CreateComponent(&m_video_source, MMAL_COMPONENT_DEFAULT_VIDEO_SPLITTER);
	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "Failed to create %s\n" PRINTF_COLOR_NONE, MMAL_COMPONENT_DEFAULT_VIDEO_SPLITTER);

		goto error;
	}

	status = SetupComponentVideoInput(&m_video_source,
								info->mmal_encoding,
								0, 0, m_fmt.fmt.pix.width, m_fmt.fmt.pix.height,
								V4L2_BUFFER_DEFAULT,
								(struct MMAL_PORT_USERDATA_T *)this,
								Splitter_Input_Port_CB);

	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "Failed to SetupComponentVideoInput()\n" PRINTF_COLOR_NONE);

		goto error;
	}

	if (m_fps)
		m_frame_time_usec = (1000000 / m_fps);

	status = SetupComponentVideoOutput(&m_video_source, NULL, 3, ReturnBuffersToPort, Splitter_Outputput_Port_CB);

	if (status != MMAL_SUCCESS)
	{
		fprintf(stderr, PRINTF_COLOR_RED "Failed to SetupComponentVideoOutput()\n" PRINTF_COLOR_NONE);

		goto error;
	}

	return status;

error:

	DestroyComponent(&m_video_source);

	return status;
}

void* CV4l2Cam::DoCapture(void* arg)
{
	CV4l2Cam* pThis = (CV4l2Cam*)arg;
	int ret = 0;
	struct pollfd fds[1];
	struct v4l2_buffer buf;
	struct v4l2_plane planes[VIDEO_MAX_PLANES];
	uint32_t i;
	int queue_buffer = 0;

	if (!pThis)
		return NULL;

	fds[0].fd = pThis->m_fd;
	fds[0].events = POLLIN | POLLPRI;

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
		else if (fds[0].revents == POLLIN)
		{
			switch (pThis->m_io_method)
			{
			case IO_METHOD_READ:
				break;
			case IO_METHOD_MMAP:
				buf.type 		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory 		= V4L2_MEMORY_MMAP;
				buf.length 		= VIDEO_MAX_PLANES;
				buf.m.planes 	= planes;

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
				const char *ts_type, *ts_source;
				queue_buffer = 1;

				pThis->get_ts_flags(buf.flags, &ts_type, &ts_source);

				fprintf(stderr, "[%s] processing [%d] %p, %u, %s/%s\n", __func__,
						buf.index, pThis->m_buffers[buf.index].start, buf.bytesused,
						ts_type, ts_source);

				if (pThis->m_video_source.input.pool)
				{
					MMAL_STATUS_T status;
					MMAL_BUFFER_HEADER_T* bh = mmal_queue_wait(pThis->m_video_source.input.pool->queue);

					if (!bh)
						fprintf(stderr, "[%s] Failed to get buffer header\n", __func__);
					else
					{
						queue_buffer = 0;
						fprintf(stderr, "[%s] V4L2 gave idx %d, MMAL expecting %d\n", __func__, ((struct buffer*)bh->user_data)->idx, buf.index);

						if (((struct buffer*)bh->user_data)->idx != buf.index)
							fprintf(stderr, "[%s] Mismatch in expected buffers. V4L2 gave idx %d, MMAL expecting %d\n", __func__, ((struct buffer*)bh->user_data)->idx, buf.index);

						bh->length = buf.length;	//Deliberately use length as MMAL wants the padding

						if (!pThis->m_starttime.tv_sec)
							pThis->m_starttime = buf.timestamp;

						struct timeval pts;
						timersub(&buf.timestamp, &pThis->m_starttime, &pts);
						//MMAL PTS is in usecs, so convert from struct timeval
						bh->pts = (pts.tv_sec * 1000) + (pts.tv_usec / 1000);
						fprintf(stderr, "[%s] buffer header's pts %lld ms\n", __func__, bh->pts);

						bh->flags = MMAL_BUFFER_HEADER_FLAG_FRAME_END;

						status = mmal_port_send_buffer(pThis->m_video_source.input.port, bh);
						if (status != MMAL_SUCCESS)
							fprintf(stderr, "[%s] mmal_port_send_buffer fail\n", __func__);
					}
				}

				if (queue_buffer && (-1 == pThis->QueueBuffer(buf.index)))
				{
					fprintf(stderr, "[%s][%s] VIDIOC_QBUF failed(%d, %s)\n", pThis->m_dev_name, __func__, errno, strerror(errno));
					pThis->StopCapture();
					break;
				}
				break;
			case IO_METHOD_USERPTR:
//				memset(&buf, 0x0, sizeof(buf));
//
//				buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
//				buf.memory 	= V4L2_MEMORY_USERPTR;
//
//				// Dequeue buffer
//				if (-1 == pThis->XIoctl(pThis->m_fd, VIDIOC_DQBUF, &buf))
//				{
//					fprintf(stderr, "[%s][%s] VIDIOC_DQBUF failed(%d, %s)\n", pThis->m_dev_name, __func__, errno, strerror(errno));
//					pThis->StopCapture();
//					break;
//				}
//
//				for (i=0 ; i < pThis->m_n_buffer ; ++i)
//				{
//					if ((unsigned long)pThis->m_buffers[i].start == buf.m.userptr && pThis->m_buffers[i].length == buf.length)
//						break;
//				}
//
//				if (buf.index >= pThis->m_n_buffer)
//				{
//					fprintf(stderr, "[%s][%s] Incorrect buffer index(%d/%d)\n", pThis->m_dev_name, __func__, buf.index, pThis->m_n_buffer);
//					pThis->StopCapture();
//					break;
//				}
//
//				// Process
//				fprintf(stderr, "[%s][%s] processing [%d] %p, %u\n",
//						pThis->m_dev_name, __func__,
//						buf.index, pThis->m_buffers[buf.index].start, buf.bytesused);
//
//				if (pThis->m_splitter.input.pool)
//				{
//					MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(pThis->m_splitter.input.pool->queue);
//					if (buffer)
//					{
//						mmal_buffer_header_mem_lock(buffer);
//
//						buffer->length = buf.length;
//
//						fprintf(stderr, "[%s][%s] buffer %p, %d\n",
//								pThis->m_dev_name, __func__,
//								buffer, buffer->alloc_size);
//
//						buffer->offset = 0;
//						memcpy(buffer->data, pThis->m_buffers[buf.index].start, buf.bytesused);
//						mmal_buffer_header_mem_unlock(buffer);
//
//						buffer->flags = MMAL_BUFFER_HEADER_FLAG_FRAME_END;
//
//						if (mmal_port_send_buffer(pThis->m_splitter.input.port, buffer)!= MMAL_SUCCESS)
//						   fprintf(stderr, "[%s][%s] Unable to send a buffer to splitter input port\n", pThis->m_dev_name, __func__);
//					}
//				}
//
//				// Queue buffer.
//				if (-1 == pThis->XIoctl(pThis->m_fd, VIDIOC_QBUF, &buf))
//				{
//					fprintf(stderr, "[%s][%s] VIDIOC_QBUF failed(%d, %s)\n", pThis->m_dev_name, __func__, errno, strerror(errno));
//					pThis->StopCapture();
//					break;
//				}
				break;
			}
		}
		else if (fds[0].revents == POLLPRI)
		{
			fprintf(stderr, "[%s][%s] POLLPRI\n", pThis->m_dev_name, __func__);
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

	m_io_method 		= IO_METHOD_MMAP;
	m_n_buffer			= 0;
	m_buffers			= NULL;
	m_fps				= 0;
	m_frame_time_usec 	= 0;
	m_zero_copy 		= MMAL_FALSE;
	m_timestamp_type	= 0;
	m_starttime.tv_sec	= 0;
	m_starttime.tv_usec	= 0;

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

	DestroyComponent(&m_video_source);

	DeInitMem();

	DeInit();
}

bool CV4l2Cam::Init(int id, const char* dev_name)
{
	m_fd = open(dev_name, O_RDWR);
	if (m_fd == -1)
	{
		fprintf(stderr, "[%s] Can not open %s\n", __func__, dev_name);
		return false;
	}

	m_dev_name = strdup(dev_name);
	fprintf(stderr, "[%s] %s is opened\n", __func__, m_dev_name);

	if (false == QueryCap())
	{
		fprintf(stderr, "[%s] %s is opened\n", __func__, m_dev_name);
		return false;
	}

	m_id = id;
	m_cam_type = CamType_V4l2;

//	Setup(1920, 1080);
//	Setup(1280, 760);
//	Setup(800, 600);
	Setup(640, 480);

	VideoGetFormat();

	QueryFps();

	CreateSplitterComponent();

	InitMem(m_fmt.fmt.pix.sizeimage);

	QueueAllBuffer();

//	StartCapture();

//	printf("===========Test Start==================\n");
//	struct component test_component;
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_VIDEO_RENDERER);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_IMAGE_DECODER);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_VIDEO_CONVERTER);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_SPLITTER);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_SCHEDULER);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_VIDEO_INJECTER);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_VIDEO_SPLITTER);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_AUDIO_RENDERER);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_MIRACAST);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_CLOCK);
//	CreateComponent(test_component, MMAL_COMPONENT_DEFAULT_CAMERA_INFO);
//	DestroyComponent(test_component);
//	printf("===========Test End==================\n");

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
	case IO_METHOD_USERPTR:
		buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == XIoctl(m_fd, VIDIOC_STREAMON, &buf_type))
		{
			ret = false;
			fprintf(stderr, "[%s][%s] VIDIOC_STREAMON failed.\n", m_dev_name, __func__);
		}

		m_do_capture = true;
		pthread_create(&m_capture_thread, NULL, DoCapture, this);
		pthread_yield();
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
