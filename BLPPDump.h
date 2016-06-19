
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/BLPPDB.h"

using namespace llvm;
class BLPPDump : public ModulePass
{
public:
  BLPPDump(); 
  virtual bool runOnModule(Module &m);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const
  {
    AU.addRequired<BLPPDB>();
  }
  virtual const char *getPassName() {return "BLPPDump";}
  static char ID; 
};
