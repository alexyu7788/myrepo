SET( CMAKE_EXE_LINKER_FLAGS "-Wl,--no-as-needed" )

SET(SRC_LIST vce.cpp
             VceCtrl.cpp
             Encoder.cpp
             Camera.cpp
             PiCam.cpp
             PiCamCtrl.cpp
             PiCamHelpers.cpp
             PiCamCli.cpp
             V4l2Cam.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/../common/component.c)

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
                      avdevice
                      avfilter
                      avcodec
                      avresample
                      avutil
                      avformat)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../common ${CMAKE_CURRENT_SOURCE_DIR} ${MODULE_INC}/ ${MODULE_INC2})

SET(TARGET_NAME vce)
ADD_EXECUTABLE(${TARGET_NAME} ${SRC_LIST})
TARGET_LINK_LIBRARIES(${TARGET_NAME} ${LINK_LIST})
