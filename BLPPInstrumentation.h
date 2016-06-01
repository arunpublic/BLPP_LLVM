#ifndef BLPPINSTRUMENTATION_H
#define BLPPINSTRUMENTATION_H

#include "llvm/Analysis/BLPP.h"
using namespace llvm;
class BLPPInstrumentation : public ModulePass
{
  protected:
    Value *psRecordEntry, *psRecordExit, *psRecordPathSum;
    void replaceTarget(TerminatorInst *psTerm, BasicBlock *psOldTarget, 
      BasicBlock *psNewTarget);
    void InstrumentFunction(Function &f, uint32_t uiProcID, BLPP &bp);
    void replacePhiUsesWith(BasicBlock *psChild,
      BasicBlock *psOldParent,  BasicBlock *psNewParent);

 
  public:
    BLPPInstrumentation();
    virtual bool runOnModule(Module &m);
    BasicBlock* splitEdge(BasicBlock *psTail, BasicBlock *psHead);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const
    {
      AU.addRequired<BLPP>();
    }
    virtual const char *getPassName() {return "BLPPInstrumentation";}
    static char ID;
    
};
#endif


