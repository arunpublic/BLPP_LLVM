##===- lib/Transforms/Hello/Makefile -----------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL = ../../..
LIBRARYNAME = LLVMPathProfiler
PARALLEL_DIRS = PPInfoSerializer
LOADABLE_MODULE = 1
LINK_LIBS_IN_SHARED = 1
USEDLIBS = BLPPAnalysis.a


# If we don't need RTTI or EH, there's no reason to export anything
# from the plugin.
ifneq ($(REQUIRES_RTTI), 1)
ifneq ($(REQUIRES_EH), 1)
EXPORTED_SYMBOL_FILE = $(PROJ_SRC_DIR)/PathProfiler.exports
endif
endif

LDFLAGS += libBLPPAnalysis.a

include $(LEVEL)/Makefile.common

