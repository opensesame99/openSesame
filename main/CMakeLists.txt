#opensesame main module CmakeLists.txt -- opensesame

set(APP_opensesame opensesame)

set(APP_opensesame_SRC
    main.cpp
    configure.cpp
    ../api/web_server.cpp
    ../api/web_server_query.cpp
    ../api/web_server_update.cpp
    ../api/web_server_command.cpp
    ../api/web_server_helper.cpp
    ../api/websocket_server.cpp
    ../api/console.cpp
)

set(INNER_LIBS opensesame_glue opensesame_contract opensesame_ledger opensesame_consensus opensesame_overlay opensesame_common opensesame_utils opensesame_proto opensesame_http opensesame_ed25519 opensesame_monitor)
set(V8_LIBS v8_base v8_libbase v8_external_snapshot v8_libplatform v8_libsampler icui18n icuuc inspector)

#Generate executable files
add_executable(${APP_opensesame} ${APP_opensesame_SRC})

#Specify dependent libraries for target objects
	
IF (${OS_NAME} MATCHES "OS_LINUX")  
    MESSAGE(STATUS "current platform: Linux ")  
	target_link_libraries(${APP_opensesame}
    -Wl,-dn ${INNER_LIBS} -Wl,--start-group ${V8_LIBS} -Wl,--end-group ${opensesame_DEPENDS_LIBS} ${opensesame_LINKER_FLAGS})
ELSE ()  
	MESSAGE(STATUS "current platform: MAC ")  
	add_definitions(${opensesame_LINKER_FLAGS})
	target_link_libraries(${APP_opensesame} ${INNER_LIBS} ${V8_LIBS} ${opensesame_DEPENDS_LIBS})
ENDIF () 


#Specify compiling options for target objets
target_compile_options(${APP_opensesame}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${APP_opensesame}
    RUNTIME DESTINATION ${opensesameCHAIN}/bin
)
