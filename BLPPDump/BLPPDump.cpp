#include "llvm/Transforms/BLPPDump.h"
#include <iostream>

BLPPDump::BLPPDump() : ModulePass(ID) {}

bool BLPPDump::runOnModule(Module &m)
{
  uint32_t i = 0;
  for (Module::iterator it = m.begin(); it != m.end(); it++)
  {
    Function &f = *it;
    if (f.isDeclaration()) continue;
    BLPPDB &bdb = getAnalysis<BLPPDB>(f);
    bdb.set_context(i);
    if (bdb.was_called(i))
      std::cout << "Function " << i << " was called\n";
    i++;
  }
  return true;
}

char BLPPDump::ID = 0;
static RegisterPass<BLPPDump> BLPPDumpRegistration ("blppdump", "dump profile info");

