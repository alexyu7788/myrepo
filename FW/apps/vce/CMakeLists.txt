SET( CMAKE_EXE_LINKER_FLAGS "-Wl,--no-as-needed" )

SET(SRC_LIST vce.cpp
             VceCtrl.cpp
             Camera.cpp
             PiCam.cpp
             PiCamCtrl.cpp
             PiCamHelpers.cpp
             PiCamCli.cpp
             V4l2Cam.cpp)

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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${MODULE_INC}/)

SET(TARGET_NAME vce)
ADD_EXECUTABLE(${TARGET_NAME} ${SRC_LIST})
TARGET_LINK_LIBRARIES(${TARGET_NAME} ${LINK_LIST})
