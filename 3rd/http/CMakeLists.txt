#opensesame http module CmakeLists.txt -- libopensesame_http.a

set(LIB_opensesame_HTTP opensesame_http)
aux_source_directory(${opensesame_SRC_DIR}/3rd/http/ HTTP_SRC)

add_library(${LIB_opensesame_HTTP} ${HTTP_SRC})

target_compile_options(${LIB_opensesame_HTTP}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D${OS_NAME}
)
