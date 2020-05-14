#opensesame ed25519 module CmakeLists.txt -- libopensesame_ed25519.a

set(LIB_opensesame_ED25519 opensesame_ed25519)
aux_source_directory(${opensesame_SRC_DIR}/3rd/ed25519-donna/ ED25519_SRC)
add_library(${LIB_opensesame_ED25519} ${ED25519_SRC}) 

target_compile_options(${LIB_opensesame_ED25519} 
	PUBLIC -DASIO_STANDALONE 
	PUBLIC -fPIC
)
