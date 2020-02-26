#ifndef MSG_BROKER_H
#define MSG_BROKER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A Structure for message transmission between processes
 */
typedef struct
{
	uint32_t     host_len;
	const char*  host;       // Max. 16 bytes

	uint32_t     cmd_len;
	const char*  cmd;        // Max. 16 bytes

	uint32_t     data_size;
	uint8_t*     data;       //max. size is 256 bytes;
	uint32_t     has_response; //0: no, 1: yes
} MsgContext;

/**
 * A function pointer for message callback function which was called in MsgBroker_Run
 */
typedef void (*MsgCallBack)(MsgContext*, void* user_data);

/**
 * @brief Function for message reciver to get message 
 *
 * @param[in] path The path to command fifo.
 * @param[in] func The function pointer for MsgCallBack.
 * @param[in] user_data User data pointer to pass in MsgCallBack function.
 * @param[in] terminate The pointer to terminate flag to inform MsgBroker to quit message listening.
 * @return The handle of resize object
 */
void MsgBroker_Run(const char* path, MsgCallBack func, void* user_data, int* terminate);

/**
 * @brief Function for message sender to send message 
 *
 * @param[in] path The path to command fifo.
 * @param[in] MsgContext The pointer to MsgContext. 
   @return Success: 0  Fail: negative integer.
 */
int MsgBroker_SendMsg(const char* path, MsgContext*);

#ifdef __cplusplus
}
#endif

#endif
