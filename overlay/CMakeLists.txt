#opensesame overlay module CmakeLists.txt -- libopensesame_overlay.a

set(LIB_opensesame_OVERLAY opensesame_overlay)
#Automatically get src files from the specified directory
aux_source_directory(${opensesame_SRC_DIR}/overlay/ OVERLAY_SRC)

#Generate static library files
add_library(${LIB_opensesame_OVERLAY} ${OVERLAY_SRC})

#Specify compiling options for the target objects
target_compile_options(${LIB_opensesame_OVERLAY}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
