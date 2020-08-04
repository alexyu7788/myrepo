#ifndef __V4l2CAMERA_H__
#define __V4l2CAMERA_H__

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <user-vcsm.h>

#include "Camera.h"
// ---------------------------------------------------------------------------------
class CV4l2Cam;
// ---------------------------------------------------------------------------------
enum io_method {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
};
// ---------------------------------------------------------------------------------
struct buffer {
	unsigned int idx;
	unsigned int padding[VIDEO_MAX_PLANES];
	unsigned int length[VIDEO_MAX_PLANES];
	void *start[VIDEO_MAX_PLANES];
	MMAL_BUFFER_HEADER_T *bufferheader;
	int dma_fd;
	unsigned int vcsm_handle;
};

struct v4l2_mmal_format_info
{
	const char *name;
	unsigned int fourcc;
	unsigned char n_planes;
	MMAL_FOURCC_T mmal_encoding;
};

struct v4l2_field_name_s{
	const char *name;
	enum v4l2_field field;
};

// ---------------------------------------------------------------------------------
class CV4l2Cam : public CCam
{
protected:
	int 					m_fd;
	char* 					m_dev_name;

	enum   io_method		m_io_method;
	uint32_t				m_n_buffer;
	struct buffer*			m_buffers;
	unsigned int			m_fps;
	unsigned int 			m_frame_time_usec;
	MMAL_BOOL_T				m_zero_copy;
	uint32_t 				m_timestamp_type;
	struct timeval 			m_starttime;

	pthread_cond_t			m_cond;
	pthread_mutex_t			m_mutex;
	pthread_t				m_capture_thread;

	struct v4l2_capability  m_caps;
	struct v4l2_cropcap 	m_cropcap;
	struct v4l2_fmtdesc 	m_fmtdesc;
	struct v4l2_format 		m_fmt;

	struct component		m_video_source;
public:

protected:
	int XIoctl(int fd, int request, void *arg);

	bool QueryCap();

	int  QueryFps();

	int VideoSetFormat(unsigned int w, unsigned int h,
							unsigned int format, unsigned int stride,
							unsigned int buffer_size, __u32 field,
							unsigned int flags);

	int VideoGetFormat();

	void get_ts_flags(uint32_t flags, const char **ts_type, const char **ts_source);

	bool InitMemRead(unsigned int buffer_size);

	bool InitMemMmap();

	bool InitMemUserPtr(unsigned int buffer_size);

	bool InitMem(unsigned int buffer_size);

	void DeInitMem();

	void DeInit();

	int QueueBuffer(unsigned int idx);

	int QueueAllBuffer();

	static void ReturnBuffersToPort(struct port_info* port_info);

	static void Splitter_Input_Port_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

	static void Splitter_Outputput_Port_CB(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

	const v4l2_mmal_format_info* V4l2FormatToMmalByFourcc(unsigned int forcc);

	int format_bpp(unsigned int  pixfmt);

	const char *v4l2_field_name(__u32 field);

	const char *v4l2_format_name(unsigned int fourcc);

	MMAL_STATUS_T CreateSplitterComponent();

	static void* DoCapture(void* arg);

public:
	CV4l2Cam();

	~CV4l2Cam();

	bool Init(int id, const char* dev_name);

	bool Setup(uint32_t width, uint32_t height);

	bool StartCapture();

	bool StopCapture();

};
#endif
