include(ExternalProject)

ExternalProject_Add(dlib_target
	URL ${CMAKE_CURRENT_SOURCE_DIR}/dlib-031920.tar.bz2
    CMAKE_ARGS
    "-DCMAKE_C_COMPILER=/opt/rpi4_32_toolchain/bin/arm-linux-gcc"
    "-DCMAKE_CXX_COMPILER=/opt/rpi4_32_toolchain/bin/arm-linux-g++"
    "-DBUILD_SHARED_LIBS=1"
    "-DCMAKE_INSTALL_PREFIX=${MODULES_OUTPUT_PATH}"
    "-DDLIB_USE_BLAS=1"
    "-DDLIB_USE_LAPACK=1"
	)
