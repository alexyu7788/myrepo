# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

SET(MY_TOOL_PATH /opt/rpi4_toolchain/usr)
SET(MY_TOOL_BIN_PATH /opt/rpi4_toolchain/usr/bin)
SET(MY_CROSS_COMPILER aarch64-linux-)


# specify the cross compiler
SET(CMAKE_C_COMPILER   ${MY_TOOL_BIN_PATH}/${MY_CROSS_COMPILER}gcc)
SET(CMAKE_CXX_COMPILER ${MY_TOOL_BIN_PATH}/${MY_CROSS_COMPILER}g++)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH ${MY_TOOL_PATH}/aarch64-buildroot-linux-uclibc/sysroot ${MY_TOOL_PATH}/aarch64-buildroot-linux-uclibc)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
