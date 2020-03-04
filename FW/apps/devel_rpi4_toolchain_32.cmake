# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

SET(MY_TOOL_PATH /opt/rpi4_32_toolchain/usr)
SET(MY_TOOL_BIN_PATH /opt/rpi4_32_toolchain/usr/bin)
SET(MY_CROSS_COMPILER arm-linux-)

SET(CMAKE_SYSTEM_PROCESSOR arm)

# specify the cross compiler
SET(CMAKE_C_COMPILER   ${MY_TOOL_BIN_PATH}/${MY_CROSS_COMPILER}gcc)
SET(CMAKE_CXX_COMPILER ${MY_TOOL_BIN_PATH}/${MY_CROSS_COMPILER}g++)
SET(CMAKE_ASM_COMPILER ${MY_TOOL_BIN_PATH}/${MY_CROSS_COMPILER}gcc)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH ${MY_TOOL_PATH}/arm-buildroot-linux-uclibcgnueabihf/sysroot ${MY_TOOL_PATH}/arm-buildroot-linux-uclibcgnueabihf)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# avoids annoying and pointless warnings from gcc
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U_FORTIFY_SOURCE")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -c")
