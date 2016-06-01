# If we don't need RTTI or EH, there's no reason to export anything
# from the hello plugin.
if( NOT LLVM_REQUIRES_RTTI )
  if( NOT LLVM_REQUIRES_EH )
    set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/PathProfiler.exports)
  endif()
endif()

if(WIN32 OR CYGWIN)
  set(LLVM_LINK_COMPONENTS Core Support)
endif()

set(LIBS
  BLPPAnalysis
)

add_llvm_loadable_module( LLVMPathProfiler
  BLPPInstrumentation.cpp
  LINK_LIBS
  ${LIBS} 
  )

add_subdirectory(PPInfoSerializer)
