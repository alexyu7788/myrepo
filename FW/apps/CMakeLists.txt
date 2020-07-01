CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(PRI4_APPS)

# DEBUG, RELEASE, MINSIZEREL, RELWITHDEBINFO
# We can use -DCMAKE_BUILD_TYPE=Debug or -DCMAKE_BUILD_TYPE=RelWithDebInfo to define something
#IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
#	MESSAGE("====== Debug mode ======")
#	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra -pedantic -pthread -g -fasynchronous-unwind-tables")
#	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Wextra -pedantic -pthread -g -fasynchronous-unwind-tables")
#	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
#	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -rdynamic")
#	SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -rdynamic")
#	ADD_DEFINITIONS("-DDEBUG")
#	SET(DEBUG_ENABLE true)
#
#	OPTION(ENABLE_ASAN "Use address sanitizer" OFF)
#	IF(ENABLE_ASAN)
#		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
#		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
#		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
#		SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
#		SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address")
#	ENDIF(ENABLE_ASAN)
#
#ELSE(CMAKE_BUILD_TYPE STREQUAL "Debug")
#	IF(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
#		MESSAGE("====== RelWithDebInfo mode ======")
#		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O3 -Wall -Wextra -pedantic -pthread -g")
#		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -O3 -Wall -Wextra -pedantic -pthread -g")
#		ADD_DEFINITIONS("-DDEBUG")
#		SET(DEBUG_ENABLE true)
#	ELSE(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
#		MESSAGE("====== Release mode ======")
#		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O3 -Wall -Wextra -pedantic -pthread")
#		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -O3 -Wall -Wextra -pedantic -pthread")
#		ADD_DEFINITIONS("-DNDEBUG")
#		SET(DEBUG_ENABLE false)
#	ENDIF(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
#ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
#
#IF(PLATFORM_TYPE STREQUAL "MOZART3S")
#	SET(MOZART3S_PLATFORM true CACHE BOOL "target to MOZART3S platform")
#ELSEIF(PLATFORM_TYPE STREQUAL "PAGANINI")
#	SET(PAGANINI_PLATFORM true CACHE BOOL "target to PAGANINI platform")
#ELSEIF(PLATFORM_TYPE STREQUAL "SCHUBERT")
#	SET(SCHUBERT_PLATFORM true)
#	ADD_DEFINITIONS("-DSCHUBERT_PLATFORM")
#ELSE()
#	SET(PESARO_PLATFORM true CACHE BOOL "target to Pesaro platform")
#ENDIF()

#SET(CMAKE_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

SET(MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../modules/install_temp_modules")
SET(MODULE_LIB "${MODULE_PATH}/lib")
SET(MODULE_INC "${MODULE_PATH}/include")
SET(MODULE_INC2 "${MODULE_PATH}/include/interface/vcsm")

#SET(MODULE_OPEN_SRC_3RD_INSTALL_DIR "${MODULE_PATH}/install_temp_open_src_3rd")

#SET(VTCS_SW_DEV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../vtcs_sw_dev_root")
#SET(VTCS_APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
#SET(VTCS_COMMON_INCLUDE_DIR "${MODULE_PATH}/vtcs_common")

SET(APPS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../apps/lib")

IF(CMAKE_CROSSCOMPILING)
	SET(HOST_CONFIG --host=arm-linux)
	#GET_FILENAME_COMPONENT(MY_TOOLCHAIN_DIR ${CMAKE_C_COMPILER} DIRECTORY) # This is for CMake > 2.8.11
	GET_FILENAME_COMPONENT(MY_TOOLCHAIN_DIR ${CMAKE_C_COMPILER} PATH) # This is for CMake <= 2.8.11
	SET(ADDITIONAL_ENV_CONFIG CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} LD=${CMAKE_LINKER} NM=${CMAKE_NM} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} STRIP=${CMAKE_STRIP} OBJDUMP=${CMAKE_OBJDUMP} PKG_CONFIG=${MY_TOOLCHAIN_DIR}/pkg-config)
ENDIF(CMAKE_CROSSCOMPILING)

#IF(SCHUBERT_PLATFORM)
#    SET(VML_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../vtcs_sdk/Vienna_VML") 
#    MESSAGE ("VML_PATH = ${VML_PATH}/lib")
#    LINK_DIRECTORIES(${MODULE_LIB} ${MODULE_OPEN_SRC_3RD_INSTALL_DIR}/lib ${VTCS_SW_DEV_ROOT}/lib ${VML_PATH}/lib)
#    IF(LIB_TYPE STREQUAL "StaticLib")
#        LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../vtcs_sdk/libs ${CMAKE_CURRENT_SOURCE_DIR}/../vtcs_sdk/Vienna_VML/lib)
#        SET(VML_LIB_LIST aac4enc
#                         autoscene
#                         autoexposure
#                         autowhitebalance
#                         coefgen
#                         datacrypto
#                         h264enc
#                         h264dec
#                         imgfrontendprocess
#                         imgsigprocess
#                         jpegdec
#                         jpegenc
#                         videocap
#                         videodisp
#                         vsabuffmgr
#                         gbengine
#                         pbengine
#                         massmemaccess
#                         memmgr)
#    ENDIF()
#ELSE()
#    LINK_DIRECTORIES(${MODULE_LIB} ${MODULE_OPEN_SRC_3RD_INSTALL_DIR}/lib ${VTCS_SW_DEV_ROOT}/lib)
#ENDIF()

LINK_DIRECTORIES(${MODULE_LIB})
INCLUDE_DIRECTORIES(${MODULE_INC})

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
SET(EXECUTABLE_OUTPUT_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

#INCLUDE(${CMAKE_MODULE_DIR}/ConditionalAddSubDir.cmake)

ADD_SUBDIRECTORY(vce)
ADD_SUBDIRECTORY(v4l2_mmal-master)

