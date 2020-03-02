#include "PiCam.h"

// Helpers
void CPiCam::display_valid_parameters(char *name, void (*app_help)(char*))
{

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

void CPiCam::set_app_name(const char *name)
{

}

const char * CPiCam::get_app_name()
{
	return NULL;
}

/**
 * Connect two specific ports together
 *
 * @param output_port Pointer the output port
 * @param input_port Pointer the input port
 * @param Pointer to a mmal connection pointer, reassigned if function successful
 * @return Returns a MMAL_STATUS_T giving result of operation
 *
 */
MMAL_STATUS_T CPiCam::connect_ports(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection)
{
   MMAL_STATUS_T status;

   status =  mmal_connection_create(connection, output_port, input_port, MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);

   if (status == MMAL_SUCCESS)
   {
      status =  mmal_connection_enable(*connection);
      if (status != MMAL_SUCCESS)
         mmal_connection_destroy(*connection);
   }

   return status;
}

/**
 * Checks if specified port is valid and enabled, then disables it
 *
 * @param port  Pointer the port
 *
 */
void CPiCam::check_disable_port(MMAL_PORT_T *port)
{
   if (port && port->is_enabled)
      mmal_port_disable(port);
}

/**
 * Handler for sigint signals
 *
 * @param signal_number ID of incoming signal.
 *
 */
void CPiCam::default_signal_handler(int signal_number)
{
   if (signal_number == SIGUSR1)
   {
      // Handle but ignore - prevents us dropping out if started in none-signal mode
      // and someone sends us the USR1 signal anyway
   }
   else
   {
      // Going to abort on all other signals
      vcos_log_error("Aborting program\n");
      exit(130);
   }

}

int CPiCam::mmal_status_to_int(MMAL_STATUS_T status)
{
	if (status == MMAL_SUCCESS)
	return 0;

	else
	{
	switch (status)
	{
	case MMAL_ENOMEM :
	vcos_log_error("Out of memory");
	break;
	case MMAL_ENOSPC :
	vcos_log_error("Out of resources (other than memory)");
	break;
	case MMAL_EINVAL:
	vcos_log_error("Argument is invalid");
	break;
	case MMAL_ENOSYS :
	vcos_log_error("Function not implemented");
	break;
	case MMAL_ENOENT :
	vcos_log_error("No such file or directory");
	break;
	case MMAL_ENXIO :
	vcos_log_error("No such device or address");
	break;
	case MMAL_EIO :
	vcos_log_error("I/O error");
	break;
	case MMAL_ESPIPE :
	vcos_log_error("Illegal seek");
	break;
	case MMAL_ECORRUPT :
	vcos_log_error("Data is corrupt \attention FIXME: not POSIX");
	break;
	case MMAL_ENOTREADY :
	vcos_log_error("Component is not ready \attention FIXME: not POSIX");
	break;
	case MMAL_ECONFIG :
	vcos_log_error("Component is not configured \attention FIXME: not POSIX");
	break;
	case MMAL_EISCONN :
	vcos_log_error("Port is already connected ");
	break;
	case MMAL_ENOTCONN :
	vcos_log_error("Port is disconnected");
	break;
	case MMAL_EAGAIN :
	vcos_log_error("Resource temporarily unavailable. Try again later");
	break;
	case MMAL_EFAULT :
	vcos_log_error("Bad address");
	break;
	default :
	vcos_log_error("Unknown status error");
	break;
	}

	return 1;
	}
}
