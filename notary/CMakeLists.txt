#opensesame main module CmakeLists.txt -- opensesame

set(APP_opensesame notary)

set(APP_opensesame_SRC
    main.cpp
    configure.cpp
	chain_rpc.cpp
	http_client.cpp
	notary_mgr.cpp
)


set(INNER_LIBS opensesame_common opensesame_utils opensesame_proto)


#Generate executable files
add_executable(${APP_opensesame} ${APP_opensesame_SRC})
#Specify dependent libraries for target objects
	

MESSAGE(STATUS "current platform: Linux ")  
target_link_libraries(${APP_opensesame}
-Wl,-dn ${INNER_LIBS} -Wl,--start-group -Wl,--end-group ${opensesame_DEPENDS_LIBS} ${opensesame_LINKER_FLAGS})



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
