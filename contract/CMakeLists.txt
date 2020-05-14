#opensesame contract module CmakeLists.txt -- libopensesame_contract.a

set(LIB_opensesame_CONTRACT opensesame_contract)

#Automatically get src files from the specified directory
aux_source_directory(${opensesame_SRC_DIR}/contract/ CONTRACT_SRC)

#Generate static library files
add_library(${LIB_opensesame_CONTRACT} ${CONTRACT_SRC})

#Specify compiling options for target objects
target_compile_options(${LIB_opensesame_CONTRACT}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)
