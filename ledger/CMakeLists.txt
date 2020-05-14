#opensesame ledger module CmakeLists.txt -- libopensesame_ledger.a

set(LIB_opensesame_LEDGER opensesame_ledger)

#Automatically get src files from the specified directory
aux_source_directory(${opensesame_SRC_DIR}/ledger/ LEDGER_SRC)

#Generate static library files
add_library(${LIB_opensesame_LEDGER} ${LEDGER_SRC})

#Specify compiling options for target objects
target_compile_options(${LIB_opensesame_LEDGER}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
