include(ExternalProject)

IF(LIB_TYPE STREQUAL "ShareLib")
SET(LIB_NAME libiniparser.so.1)
SET(LIB_LINK_NAME libiniparser.so)
#SET(HEADER src/dictionary.h src/iniparser.h)
SET(RUN_COMMAND mkdir -p <INSTALL_DIR>/lib && mkdir -p <INSTALL_DIR>/include/iniparser && install -m 755 <SOURCE_DIR>/${LIB_NAME} <INSTALL_DIR>/lib && install -m 644 <SOURCE_DIR>/src/dictionary.h <SOURCE_DIR>/src/iniparser.h <INSTALL_DIR>/include/iniparser && rm -f <INSTALL_DIR>/lib/${LIB_LINK_NAME} && ln -s ${LIB_NAME} <INSTALL_DIR>/lib/${LIB_LINK_NAME})
ELSE(LIB_TYPE STREQUAL "StaticLib")
SET(LIB_NAME libiniparser.a)
SET(LIB_LINK_NAME libiniparser.so)
#SET(HEADER src/dictionary.h src/iniparser.h)
SET(RUN_COMMAND mkdir -p <INSTALL_DIR>/lib && mkdir -p <INSTALL_DIR>/include/iniparser && install -m 755 <SOURCE_DIR>/${LIB_NAME} <INSTALL_DIR>/lib && install -m 644 <SOURCE_DIR>/src/dictionary.h <SOURCE_DIR>/src/iniparser.h <INSTALL_DIR>/include/iniparser && rm -f <INSTALL_DIR>/lib/${LIB_LINK_NAME})
ENDIF()

ExternalProject_Add(iniparser_target
	URL ${CMAKE_CURRENT_SOURCE_DIR}/iniparser-4.1.tar.gz
#	PATCH_COMMAND patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/iniparser_make.patch
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE 1
	BUILD_COMMAND ${ADDITIONAL_ENV_CONFIG} make
    INSTALL_DIR ${MODULES_OUTPUT_PATH}
    INSTALL_COMMAND ${RUN_COMMAND}
	)

