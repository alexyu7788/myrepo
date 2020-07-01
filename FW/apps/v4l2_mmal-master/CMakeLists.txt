SET( CMAKE_EXE_LINKER_FLAGS "-Wl,--no-as-needed" )

SET(SRC_LIST v4l2_mmal.c)

LIST(APPEND LINK_LIST msgbroker 
                      iniparser
                      mmal_core
                      mmal_util
                      mmal_vc_client
                      mmal_components
                      containers
                      vchiq_arm
                      vcos
                      vcsm
                      bcm_host
                      )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${MODULE_INC}/ ${MODULE_INC2})

SET(TARGET_NAME v4l2_mmal)
ADD_EXECUTABLE(${TARGET_NAME} ${SRC_LIST})
TARGET_LINK_LIBRARIES(${TARGET_NAME} ${LINK_LIST})
