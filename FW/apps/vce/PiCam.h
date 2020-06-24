#ifndef __PiCAMERA_H__
#define __PiCAMERA_H__

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/mmal_parameters_camera.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "interface/vmcs_host/vc_vchi_gencmd.h"

#ifdef __cplusplus
}
#endif

#include "Camera.h"

//-----------------------------------------------------------------------------------------------------
// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

// Port configuration for the splitter component
#define SPLITTER_OUTPUT_PORT 0
#define SPLITTER_PREVIEW_PORT 1

// Video format information
// 0 implies variable
#define VIDEO_FRAME_RATE_NUM 30
#define VIDEO_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

// Max bitrate we allow for recording
const int MAX_BITRATE_MJPEG = 25000000; // 25Mbits/s
const int MAX_BITRATE_LEVEL4 = 25000000; // 25Mbits/s
const int MAX_BITRATE_LEVEL42 = 62500000; // 62.5Mbits/s

/// Interval at which we check for an failure abort during capture
const int ABORT_INTERVAL = 100; // ms


/// Capture/Pause switch method
/// Simply capture for time specified
enum
{
   WAIT_METHOD_NONE,       /// Simply capture for time specified
   WAIT_METHOD_TIMED,      /// Cycle between capture and pause for times specified
   WAIT_METHOD_KEYPRESS,   /// Switch between capture and pause on keypress
   WAIT_METHOD_SIGNAL,     /// Switch between capture and pause on signal
   WAIT_METHOD_FOREVER     /// Run/record forever
};

/** Possible raw output formats
 */
typedef enum
{
   RAW_OUTPUT_FMT_YUV = 0,
   RAW_OUTPUT_FMT_RGB,
   RAW_OUTPUT_FMT_GRAY,
} RAW_OUTPUT_FMT;

typedef enum
{
   ZOOM_IN, ZOOM_OUT, ZOOM_RESET
} ZOOM_COMMAND_T;

typedef struct
{
   int id;
   char *command;
   char *abbrev;
   char *help;
   int num_parameters;
} COMMAND_LIST;

/// Cross reference structure, mode string against mode id
typedef struct xref_t
{
   const char *mode;
   int mmal_mode;
} XREF_T;

typedef struct param_float_rect_s
{
   double x;
   double y;
   double w;
   double h;
} PARAM_FLOAT_RECT_T;

typedef struct mmal_param_thumbnail_config_s
{
   int enable;
   int width,height;
   int quality;
} MMAL_PARAM_THUMBNAIL_CONFIG_T;

typedef struct
{
   char camera_name[MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN]; // Name of the camera sensor
   int width;                          /// Requested width of image
   int height;                         /// requested height of image
   char *filename;                     /// filename of output file
   int cameraNum;                      /// Camera number
   int sensor_mode;                    /// Sensor mode. 0=auto. Check docs/forum for modes selected by other values.
   int verbose;                        /// !0 if want detailed run information
   int gps;                            /// Add real-time gpsd output to output
} RASPICOMMONSETTINGS_PARAMETERS;

// There isn't actually a MMAL structure for the following, so make one
typedef struct mmal_param_colourfx_s
{
   int enable;       /// Turn colourFX on or off
   int u,v;          /// U and V to use
} MMAL_PARAM_COLOURFX_T;

/** Struct used to pass information in encoder port userdata to callback
 */
typedef struct
{
   FILE *file_handle;                   /// File handle to write buffer data to.
   struct RASPIVID_STATE_S *pstate;     /// pointer to our state in case required in callback
   int abort;                           /// Set to 1 in callback if an error occurs to attempt to abort the capture
   char *cb_buff;                       /// Circular buffer
   int   cb_len;                        /// Length of buffer
   int   cb_wptr;                       /// Current write pointer
   int   cb_wrap;                       /// Has buffer wrapped at least once?
   int   cb_data;                       /// Valid bytes in buffer
#define IFRAME_BUFSIZE (60*1000)
   int   iframe_buff[IFRAME_BUFSIZE];          /// buffer of iframe pointers
   int   iframe_buff_wpos;
   int   iframe_buff_rpos;
   char  header_bytes[29];
   int  header_wptr;
   FILE *imv_file_handle;               /// File handle to write inline motion vectors to.
   FILE *raw_file_handle;               /// File handle to write raw data to.
   int  flush_buffers;
   FILE *pts_file_handle;               /// File timestamps
} PORT_USERDATA;

typedef struct
{
   int wantPreview;                       /// Display a preview
   int wantFullScreenPreview;             /// 0 is use previewRect, non-zero to use full screen
   int opacity;                           /// Opacity of window - 0 = transparent, 255 = opaque
   MMAL_RECT_T previewWindow;             /// Destination rectangle for the preview window.
   MMAL_COMPONENT_T *preview_component;   /// Pointer to the created preview display component
   int display_num;                       /// Display number to use.
} RASPIPREVIEW_PARAMETERS;

/// struct contain camera settings
typedef struct raspicam_camera_parameters_s
{
   int sharpness;             /// -100 to 100
   int contrast;              /// -100 to 100
   int brightness;            ///  0 to 100
   int saturation;            ///  -100 to 100
   int ISO;                   ///  TODO : what range?
   int videoStabilisation;    /// 0 or 1 (false or true)
   int exposureCompensation;  /// -10 to +10 ?
   MMAL_PARAM_EXPOSUREMODE_T exposureMode;
   MMAL_PARAM_EXPOSUREMETERINGMODE_T exposureMeterMode;
   MMAL_PARAM_AWBMODE_T awbMode;
   MMAL_PARAM_IMAGEFX_T imageEffect;
   MMAL_PARAMETER_IMAGEFX_PARAMETERS_T imageEffectsParameters;
   MMAL_PARAM_COLOURFX_T colourEffects;
   MMAL_PARAM_FLICKERAVOID_T flickerAvoidMode;
   int rotation;              /// 0-359
   int hflip;                 /// 0 or 1
   int vflip;                 /// 0 or 1
   PARAM_FLOAT_RECT_T  roi;   /// region of interest to use on the sensor. Normalised [0,1] values in the rect
   int shutter_speed;         /// 0 = auto, otherwise the shutter speed in ms
   float awb_gains_r;         /// AWB red gain
   float awb_gains_b;         /// AWB blue gain
   MMAL_PARAMETER_DRC_STRENGTH_T drc_level;  // Strength of Dynamic Range compression to apply
   MMAL_BOOL_T stats_pass;    /// Stills capture statistics pass on/off
   int enable_annotate;       /// Flag to enable the annotate, 0 = disabled, otherwise a bitmask of what needs to be displayed
   char annotate_string[MMAL_CAMERA_ANNOTATE_MAX_TEXT_LEN_V2]; /// String to use for annotate - overrides certain bitmask settings
   int annotate_text_size;    // Text size for annotation
   int annotate_text_colour;  // Text colour for annotation
   int annotate_bg_colour;    // Background colour for annotation
   unsigned int annotate_justify;
   unsigned int annotate_x;
   unsigned int annotate_y;

   MMAL_PARAMETER_STEREOSCOPIC_MODE_T stereo_mode;
   float analog_gain;         // Analog gain
   float digital_gain;        // Digital gain

   int settings;
} RASPICAM_CAMERA_PARAMETERS;

/** Structure containing all state information for the current run
 */
struct RASPIVID_STATE_S
{
   RASPICOMMONSETTINGS_PARAMETERS common_settings;     /// Common settings
   int timeout;                        /// Time taken before frame is grabbed and app then shuts down. Units are milliseconds
   MMAL_FOURCC_T encoding;             /// Requested codec video encoding (MJPEG or H264)
   int bitrate;                        /// Requested bitrate
   int framerate;                      /// Requested frame rate (fps)
   int intraperiod;                    /// Intra-refresh period (key frame rate)
   int quantisationParameter;          /// Quantisation parameter - quality. Set bitrate 0 and set this for variable bitrate
   int bInlineHeaders;                  /// Insert inline headers to stream (SPS, PPS)
   int demoMode;                       /// Run app in demo mode
   int demoInterval;                   /// Interval between camera settings changes
   int immutableInput;                 /// Flag to specify whether encoder works in place or creates a new buffer. Result is preview can display either
   /// the camera output or the encoder output (with compression artifacts)
   int profile;                        /// H264 profile to use for encoding
   int level;                          /// H264 level to use for encoding
   int waitMethod;                     /// Method for switching between pause and capture

   int onTime;                         /// In timed cycle mode, the amount of time the capture is on per cycle
   int offTime;                        /// In timed cycle mode, the amount of time the capture is off per cycle

   int segmentSize;                    /// Segment mode In timed cycle mode, the amount of time the capture is off per cycle
   int segmentWrap;                    /// Point at which to wrap segment counter
   int segmentNumber;                  /// Current segment counter
   int splitNow;                       /// Split at next possible i-frame if set to 1.
   int splitWait;                      /// Switch if user wants splited files

   RASPIPREVIEW_PARAMETERS preview_parameters;   /// Preview setup parameters
   RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters

   MMAL_COMPONENT_T *camera_component;    /// Pointer to the camera component
   MMAL_COMPONENT_T *splitter_component;  /// Pointer to the splitter component
   MMAL_COMPONENT_T *encoder_component;   /// Pointer to the encoder component
   MMAL_CONNECTION_T *preview_connection; /// Pointer to the connection from camera or splitter to preview
   MMAL_CONNECTION_T *splitter_connection;/// Pointer to the connection from camera to splitter
   MMAL_CONNECTION_T *encoder_connection; /// Pointer to the connection from camera to encoder

   MMAL_POOL_T *splitter_pool; /// Pointer to the pool of buffers used by splitter output port 0
   MMAL_POOL_T *encoder_pool; /// Pointer to the pool of buffers used by encoder output port

   PORT_USERDATA callback_data;        /// Used to move data to the encoder callback

   int bCapturing;                     /// State of capture/pause
   int bCircularBuffer;                /// Whether we are writing to a circular buffer

   int inlineMotionVectors;             /// Encoder outputs inline Motion Vectors
   char *imv_filename;                  /// filename of inline Motion Vectors output
   int raw_output;                      /// Output raw video from camera as well
   RAW_OUTPUT_FMT raw_output_fmt;       /// The raw video format
   char *raw_filename;                  /// Filename for raw video output
   int intra_refresh_type;              /// What intra refresh type to use. -1 to not set.
   int frame;
   char *pts_filename;
   int save_pts;
   int64_t starttime;
   int64_t lasttime;

   bool netListen;
   MMAL_BOOL_T addSPSTiming;
   int slices;
};

//------------------------------------------------------------------------------------------------------

class CPiCam : public CCam
{
protected:

	RASPICOMMONSETTINGS_PARAMETERS 	m_common_settings;     				/// Common settings
	int 							m_timeout;                        /// Time taken before frame is grabbed and app then shuts down. Units are milliseconds
	MMAL_FOURCC_T 					m_encoding;             			/// Requested codec video encoding (MJPEG or H264)
	int 							m_bitrate;                        /// Requested bitrate
	int 							m_framerate;                      /// Requested frame rate (fps)
	int 							m_intraperiod;                    /// Intra-refresh period (key frame rate)
	int 							m_quantisationParameter;          /// Quantisation parameter - quality. Set bitrate 0 and set this for variable bitrate
	int 							m_bInlineHeaders;                  /// Insert inline headers to stream (SPS, PPS)
	int 							m_demoMode;                       /// Run app in demo mode
	int 							m_demoInterval;                   /// Interval between camera settings changes
	int 							m_immutableInput;                 /// Flag to specify whether encoder works in place or creates a new buffer. Result is preview can display either
																		/// the camera output or the encoder output (with compression artifacts)
	int 							m_profile;                        /// H264 profile to use for encoding
	int 							m_level;                          /// H264 level to use for encoding
	int 							m_waitMethod;                     /// Method for switching between pause and capture

	int 							m_onTime;                         /// In timed cycle mode, the amount of time the capture is on per cycle
	int 							m_offTime;                        /// In timed cycle mode, the amount of time the capture is off per cycle

	int 							m_segmentSize;                    /// Segment mode In timed cycle mode, the amount of time the capture is off per cycle
	int 							m_segmentWrap;                    /// Point at which to wrap segment counter
	int 							m_segmentNumber;                  /// Current segment counter
	int 							m_splitNow;                       /// Split at next possible i-frame if set to 1.
	int 							m_splitWait;                      /// Switch if user wants splited files

	RASPIPREVIEW_PARAMETERS 		m_preview_parameters;   /// Preview setup parameters
	RASPICAM_CAMERA_PARAMETERS 		m_camera_parameters; /// Camera setup parameters

	MMAL_COMPONENT_T *				m_camera_component;    /// Pointer to the camera component
	MMAL_COMPONENT_T *				m_splitter_component;  /// Pointer to the splitter component
	MMAL_COMPONENT_T *				m_encoder_component;   /// Pointer to the encoder component
	MMAL_CONNECTION_T *				m_preview_connection; /// Pointer to the connection from camera or splitter to preview
	MMAL_CONNECTION_T *				m_splitter_connection;/// Pointer to the connection from camera to splitter
	MMAL_CONNECTION_T *				m_encoder_connection; /// Pointer to the connection from camera to encoder

	MMAL_POOL_T *					m_splitter_pool; /// Pointer to the pool of buffers used by splitter output port 0
	MMAL_POOL_T *					m_encoder_pool; /// Pointer to the pool of buffers used by encoder output port

	PORT_USERDATA 					m_callback_data;        /// Used to move data to the encoder callback

	int 							m_bCapturing;                     /// State of capture/pause
	int 							m_bCircularBuffer;                /// Whether we are writing to a circular buffer

	int 							m_inlineMotionVectors;             /// Encoder outputs inline Motion Vectors
	char *							m_imv_filename;                  /// filename of inline Motion Vectors output
	int 							m_raw_output;                      /// Output raw video from camera as well
	RAW_OUTPUT_FMT 					m_raw_output_fmt;       /// The raw video format
	char *							m_raw_filename;                  /// Filename for raw video output
	int 							m_intra_refresh_type;              /// What intra refresh type to use. -1 to not set.
	int 							m_frame;
	char *							m_pts_filename;
	int 							m_save_pts;
	int64_t 						m_starttime;
	int64_t 						m_lasttime;

	bool 							m_netListen;
	MMAL_BOOL_T 					m_addSPSTiming;
	int 							m_slices;

public:

protected:

	void preview_set_defaults();

	void control_set_defaults();

	void commonsettings_set_defaults();

	void default_status();

	void check_camera_model(int cam_num);

	MMAL_STATUS_T create_camera_component();

	void destroy_camera_component();

	// Control
	void control_check_configuration(int min_gpu_mem);
	int control_parse_cmdline(RASPICAM_CAMERA_PARAMETERS *params, const char *arg1, const char *arg2);
	void control_display_help();
	int control_cycle_test(MMAL_COMPONENT_T *camera);

	int control_set_all_parameters(MMAL_COMPONENT_T *camera, const RASPICAM_CAMERA_PARAMETERS *params);
	int control_get_all_parameters(MMAL_COMPONENT_T *camera, RASPICAM_CAMERA_PARAMETERS *params);
	void control_dump_parameters(const RASPICAM_CAMERA_PARAMETERS *params);

	void control_set_defaults(RASPICAM_CAMERA_PARAMETERS *params);

	// Individual setting functions
	int control_set_saturation(MMAL_COMPONENT_T *camera, int saturation);
	int control_set_sharpness(MMAL_COMPONENT_T *camera, int sharpness);
	int control_set_contrast(MMAL_COMPONENT_T *camera, int contrast);
	int control_set_brightness(MMAL_COMPONENT_T *camera, int brightness);
	int control_set_ISO(MMAL_COMPONENT_T *camera, int ISO);
	int control_set_metering_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_EXPOSUREMETERINGMODE_T mode);
	int control_set_video_stabilisation(MMAL_COMPONENT_T *camera, int vstabilisation);
	int control_set_exposure_compensation(MMAL_COMPONENT_T *camera, int exp_comp);
	int control_set_exposure_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_EXPOSUREMODE_T mode);
	int control_set_flicker_avoid_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_FLICKERAVOID_T mode);
	int control_set_awb_mode(MMAL_COMPONENT_T *camera, MMAL_PARAM_AWBMODE_T awb_mode);
	int control_set_awb_gains(MMAL_COMPONENT_T *camera, float r_gain, float b_gain);
	int control_set_imageFX(MMAL_COMPONENT_T *camera, MMAL_PARAM_IMAGEFX_T imageFX);
	int control_set_colourFX(MMAL_COMPONENT_T *camera, const MMAL_PARAM_COLOURFX_T *colourFX);
	int control_set_rotation(MMAL_COMPONENT_T *camera, int rotation);
	int control_set_flips(MMAL_COMPONENT_T *camera, int hflip, int vflip);
	int control_set_ROI(MMAL_COMPONENT_T *camera, PARAM_FLOAT_RECT_T rect);
	int control_zoom_in_zoom_out(MMAL_COMPONENT_T *camera, ZOOM_COMMAND_T zoom_command, PARAM_FLOAT_RECT_T *roi);
	int control_set_shutter_speed(MMAL_COMPONENT_T *camera, int speed_ms);
	int control_set_DRC(MMAL_COMPONENT_T *camera, MMAL_PARAMETER_DRC_STRENGTH_T strength);
	int control_set_stats_pass(MMAL_COMPONENT_T *camera, int stats_pass);
	int control_set_annotate(MMAL_COMPONENT_T *camera, const int settings, const char *string,
	                                 const int text_size, const int text_colour, const int bg_colour,
	                                 const unsigned int justify, const unsigned int x, const unsigned int y);
	int control_set_stereo_mode(MMAL_PORT_T *port, MMAL_PARAMETER_STEREOSCOPIC_MODE_T *stereo_mode);
	int control_set_gains(MMAL_COMPONENT_T *camera, float analog, float digital);
	int control_get_saturation(MMAL_COMPONENT_T *camera);
	int control_get_sharpness(MMAL_COMPONENT_T *camera);
	int control_get_contrast(MMAL_COMPONENT_T *camera);
	int control_get_brightness(MMAL_COMPONENT_T *camera);
	int control_get_ISO(MMAL_COMPONENT_T *camera);
	MMAL_PARAM_EXPOSUREMETERINGMODE_T control_get_metering_mode(MMAL_COMPONENT_T *camera);
	int control_get_video_stabilisation(MMAL_COMPONENT_T *camera);
	int control_get_exposure_compensation(MMAL_COMPONENT_T *camera);
	MMAL_PARAM_THUMBNAIL_CONFIG_T control_get_thumbnail_parameters(MMAL_COMPONENT_T *camera);
	MMAL_PARAM_EXPOSUREMODE_T control_get_exposure_mode(MMAL_COMPONENT_T *camera);
	MMAL_PARAM_FLICKERAVOID_T control_get_flicker_avoid_mode(MMAL_COMPONENT_T *camera);
	MMAL_PARAM_AWBMODE_T control_get_awb_mode(MMAL_COMPONENT_T *camera);
	MMAL_PARAM_IMAGEFX_T control_get_imageFX(MMAL_COMPONENT_T *camera);
	MMAL_PARAM_COLOURFX_T control_get_colourFX(MMAL_COMPONENT_T *camera);

	static void default_camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

	int control_get_mem_gpu(void);
	void control_get_camera(int *supported, int *detected);

	// Helpers
	void display_valid_parameters(char *name, void (*app_help)(char*));
	void get_sensor_defaults(int camera_num, char *camera_name, int *width, int *height);
	void set_app_name(const char *name);
	const char *get_app_name();
	MMAL_STATUS_T connect_ports(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection);
	void check_disable_port(MMAL_PORT_T *port);
	void default_signal_handler(int signal_number);
	int mmal_status_to_int(MMAL_STATUS_T status);
	void print_app_details(FILE *fd);
	uint64_t get_microseconds64();

	// CLI
	void cli_display_help(const COMMAND_LIST *commands, const int num_commands);
	int cli_get_command_id(const COMMAND_LIST *commands, const int num_commands, const char *arg, int *num_parameters);

	int cli_map_xref(const char *str, const XREF_T *map, int num_refs);
	const char *cli_unmap_xref(const int en, XREF_T *map, int num_refs);

public:
	CPiCam();

	~CPiCam();

	bool Init(const char* dev_name);

	bool Setup(uint32_t width, uint32_t height);

	bool StartCapture();

	bool StopCapture();
};
#endif
