cmake_minimum_required(VERSION 2.8)

project(opensesame)
add_compile_options(-g -O2)
add_definitions(-DSVNVERSION=\"${SVNVERSION}\")
message(STATUS "-DSVNVERSION="${SVNVERSION})

set( V8_LIB_PATH "linux")
set( OS_NAME "OS_LINUX")
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
    MESSAGE(STATUS "current platform: Linux ")  
	set( OS_NAME "OS_LINUX")
	set(opensesame_LINKER_FLAGS "-Wl,-dy -lpthread -lrt -ldl")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")  
    MESSAGE(STATUS "current platform: Darwin")  
	set( OS_NAME "OS_MAC")
	set(opensesame_LINKER_FLAGS "-Wl, -lpthread -ldl")
	set( V8_LIB_PATH "mac")
ELSE ()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
ENDIF ()  

set(opensesameCHAIN SesameChain)
set(opensesame_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(opensesame_ROOT_DIR ${opensesame_SRC_DIR}/..)
set(LIBRARY_OUTPUT_PATH ${opensesame_ROOT_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${opensesame_ROOT_DIR}/bin)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

execute_process(COMMAND make all WORKING_DIRECTORY ${opensesame_SRC_DIR}/3rd)

include_directories(
    ${opensesame_SRC_DIR}
    ${opensesame_SRC_DIR}/3rd/basic/include/v8
    ${opensesame_SRC_DIR}/3rd/protobuf/src
    ${opensesame_SRC_DIR}/3rd/jsoncpp/include
    ${opensesame_SRC_DIR}/3rd/sqlite 
    ${opensesame_SRC_DIR}/3rd/rocksdb/include
    ${opensesame_SRC_DIR}/3rd/pcre-8.39
    ${opensesame_SRC_DIR}/3rd/websocketpp
    ${opensesame_SRC_DIR}/3rd/bzip2-1.0.6
    ${opensesame_SRC_DIR}/3rd/zlib-1.2.8
    ${opensesame_SRC_DIR}/3rd/asio/include
    ${opensesame_SRC_DIR}/3rd/openssl/include
    ${opensesame_SRC_DIR}/3rd/http
    ${opensesame_SRC_DIR}/3rd/libscrypt
)

link_directories(
    ${opensesame_SRC_DIR}/3rd/protobuf/src/.libs
    ${opensesame_SRC_DIR}/3rd/rocksdb
    ${opensesame_SRC_DIR}/3rd/pcre-8.39/.libs
    ${opensesame_SRC_DIR}/3rd/jsoncpp/libs
    ${opensesame_SRC_DIR}/3rd/openssl
    ${opensesame_SRC_DIR}/3rd/zlib-1.2.8
    ${opensesame_SRC_DIR}/3rd/libscrypt
    ${opensesame_SRC_DIR}/3rd/bzip2-1.0.6
    ${opensesame_SRC_DIR}/3rd/basic/lib
    ${opensesame_SRC_DIR}/3rd/curl/lib/.libs/
    ${opensesame_SRC_DIR}/3rd/v8_target/${V8_LIB_PATH}/
    ${LIBRARY_OUTPUT_PATH}
)

set(opensesame_DEPENDS_LIBS protobuf rocksdb pcreposix pcrecpp pcre json ssl crypto z bz2 scrypt)

add_subdirectory(3rd/http)
add_subdirectory(3rd/ed25519-donna)
add_subdirectory(glue)
add_subdirectory(utils)
add_subdirectory(proto)
add_subdirectory(contract)
add_subdirectory(ledger)
add_subdirectory(common)
add_subdirectory(libopensesame_tools)
add_subdirectory(overlay)
add_subdirectory(consensus)
add_subdirectory(monitor)
add_subdirectory(main)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
	add_subdirectory(daemon)
	set(opensesame_SCRIPTS ${opensesame_ROOT_DIR}/deploy)
	install(
		PROGRAMS ${opensesame_SCRIPTS}/opensesame ${opensesame_SCRIPTS}/opensesamed ${opensesame_SCRIPTS}/start-stop-daemon
		DESTINATION ${opensesameCHAIN}/scripts
	)
ENDIF ()  

install(
    DIRECTORY ${opensesame_ROOT_DIR}/bin/
    DESTINATION ${opensesameCHAIN}/bin
    FILES_MATCHING PATTERN "*.bin" PATTERN "*.dat"
)

install(
    DIRECTORY ${opensesame_ROOT_DIR}/build/win32/config/
    DESTINATION ${opensesameCHAIN}/config
    FILES_MATCHING PATTERN "opensesame.json" PATTERN "opensesame-mainnet.json" PATTERN "opensesame-testnet.json" PATTERN "opensesame-single.json"
)

install(
    DIRECTORY ${opensesame_ROOT_DIR}/build/win32/jslib/
    DESTINATION ${opensesameCHAIN}/jslib
    FILES_MATCHING PATTERN "*.js"
)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
	set(SUPPLEMENT ${opensesame_ROOT_DIR}/build/linux/MakeSupplement)
	set(FIX_DIR ${CMAKE_INSTALL_PREFIX}/${opensesameCHAIN})
	message(STATUS "install dir: ${FIX_DIR}")

	file(WRITE ${SUPPLEMENT} "soft_link:\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s ${FIX_DIR}/scripts/opensesame /etc/init.d/opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s ${FIX_DIR}/scripts/opensesamed /etc/init.d/opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesame /etc/rc1.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesame /etc/rc1.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesame /etc/rc2.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesame /etc/rc3.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesame /etc/rc4.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesame /etc/rc5.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesamed /etc/rc1.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesamed /etc/rc1.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesamed /etc/rc2.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesamed /etc/rc3.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesamed /etc/rc4.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/opensesamed /etc/rc5.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/data;\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/coredump;\n")

	file(APPEND ${SUPPLEMENT} "\nuninstall:\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc1.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc2.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc3.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc4.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc5.d/S99opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/init.d/opensesame;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc1.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc2.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc3.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc4.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc5.d/S50opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/init.d/opensesamed;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/bin;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/cfca;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/config;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/scripts\n")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")  
    set(SUPPLEMENT ${opensesame_ROOT_DIR}/build/mac/MakeSupplement)
	set(FIX_DIR ${CMAKE_INSTALL_PREFIX}/${opensesameCHAIN})
	message(STATUS "install dir: ${FIX_DIR}")

	file(WRITE ${SUPPLEMENT} "soft_link:\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/data;\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/coredump;\n")

	file(APPEND ${SUPPLEMENT} "\nuninstall:\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/bin;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/cfca;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/config;\\\n")
ELSE ()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
ENDIF ()  
