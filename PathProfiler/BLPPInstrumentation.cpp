#include "llvm/Transforms/BLPPInstrumentation.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
using namespace llvm;
BLPPInstrumentation::BLPPInstrumentation() : ModulePass(ID)
{
  psRecordEntry = psRecordExit = psRecordPathSum = nullptr;
}

void BLPPInstrumentation::replacePhiUsesWith(BasicBlock *psChild,
  BasicBlock *psOldParent,  BasicBlock *psNewParent)
{
  for (BasicBlock::iterator II = psChild->begin(), IE = psChild->end(); II != IE; ++II) {
    PHINode *PN = dyn_cast<PHINode>(II);
    if (!PN)
      break;
    int i;
    while ((i = PN->getBasicBlockIndex(psOldParent)) >= 0)
      PN->setIncomingBlock(i, psNewParent);
  }
}

void BLPPInstrumentation::replaceTarget(TerminatorInst *psTerm, 
  BasicBlock *psOldTarget,  BasicBlock *psNewTarget)
{
  bool bMultiEdge = false;
  for (uint32_t i = 0, uiNSucc = psTerm->getNumSuccessors();
    i < uiNSucc; i++)
  {
    if (psTerm->getSuccessor(i) == psOldTarget)
    {
      assert(!bMultiEdge);
      psTerm->setSuccessor(i, psNewTarget);
      replacePhiUsesWith(psOldTarget, psTerm->getParent(), psNewTarget);
      bMultiEdge = true;
    }
  } 
}

void BLPPInstrumentation::InstrumentFunction(Function &f, uint32_t uiProcID,
  BLPP &bp)
{
  LLVMContext &sContext = f.getContext();
  BasicBlock &sFront = f.getEntryBlock();
  IntegerType *psInt64Ty = IntegerType::get(sContext, 64);
  IntegerType *psInt32Ty = IntegerType::get(sContext, 32);
  Value *psProcID = ConstantInt::get(psInt32Ty, uiProcID);
  Value *psPathSumVar = new AllocaInst(psInt64Ty, "pathsum", sFront.getFirstNonPHI());
  ArrayRef<Value*> sRef3(&psProcID, 1);
  
  CallInst::Create(psRecordEntry, sRef3, "", sFront.getFirstNonPHI());
  /* Insert instrumentation code on relevant edges */
  for (std::list<BLPPEdge*>::iterator it = bp.lEdges.begin(); 
    it != bp.lEdges.end(); it++)
  {
    BLPPEdge *psEdge = *it;
    BasicBlock *psHead, *psTail;
    if (psEdge->beDummyMatchP)
    {
      assert(psEdge->atKind != PATH_SUM_INIT);
      if (psEdge->nodeHeadP->vNodeDataP)
      {
        /* BLPP edge is from entry to an internal node. CFG edge is from 
           tail of dummy match to this edge's head
         */
        psTail = static_cast<BasicBlock*>
          (psEdge->beDummyMatchP->nodeTailP->vNodeDataP);
        psHead = static_cast<BasicBlock*>
          (psEdge->nodeHeadP->vNodeDataP);
      }
      else
      {
        /* BLPP edge is from internal node to exit node. CFG edge is from
           tail of this edge to head of dummy match
        */
        psTail = static_cast<BasicBlock*>
          (psEdge->nodeTailP->vNodeDataP);
        psHead = static_cast<BasicBlock*>
          (psEdge->beDummyMatchP->nodeHeadP->vNodeDataP);
      }
    }
    else
    {
      psHead = static_cast<BasicBlock*>(psEdge->nodeHeadP->vNodeDataP);
      psTail = static_cast<BasicBlock*>(psEdge->nodeTailP->vNodeDataP);
    }
    switch(psEdge->atKind)
    {
      case INVALID:
      {
        break;
      }
      case PATH_SUM_INIT:
      {
        BasicBlock *psInsertionBlock = splitEdge(psTail, psHead);
        Instruction *psInsertionPt = psInsertionBlock->getTerminator();
        new StoreInst
          (ConstantInt::get(psInt64Ty, psEdge->siIncrement),
          psPathSumVar, psInsertionPt);
        break;
      }
      default:
      {
        BasicBlock *psNewInsertionBlock = splitEdge(psTail, psHead);
        Instruction *psInsertionPt = psNewInsertionBlock->getTerminator();
        Value *psCurPathSum = new LoadInst(psPathSumVar, "", psInsertionPt);
        if (psEdge->siIncrement)
        {
          psCurPathSum = BinaryOperator::Create(Instruction::BinaryOps::Add, 
            psCurPathSum, ConstantInt::get(psInt64Ty, psEdge->siIncrement),
            "", psInsertionPt);
          if (PATH_SUM_INCR == psEdge->atKind)
          {
            /* Save path sum */
            new StoreInst(psCurPathSum, psPathSumVar, psInsertionPt);
          }
        }
        if (PATH_SUM_READ == psEdge->atKind)
        {
          /* PathID, ProcID */
          Value* apsArgs[2] = {psCurPathSum, psProcID};
          ArrayRef<Value*> sRef(apsArgs, 2);
          CallInst::Create(psRecordPathSum, sRef, "", 
            psInsertionPt);
          if (psEdge->isReset)
          {
            /* Initialize path sum */
            new StoreInst
              (ConstantInt::get(psInt64Ty, psEdge->siReset),
              psPathSumVar, psInsertionPt);
          }
        }
        break;
      }
    }
  }
  /* serialize path profile data */
  for (std::list<BLPPEdge*>::iterator it = bp.bnExitP->lInEdges.begin(); 
   it != bp.bnExitP->lInEdges.end(); it++)  
  {
    BLPPEdge *psEdge = *it;
    if (!psEdge->beDummyMatchP)
    {
      BasicBlock *psExit = static_cast<BasicBlock*>
        (psEdge->nodeTailP->vNodeDataP);
      CallInst::Create(psRecordExit, ArrayRef<Value*>(&psProcID, 1),
        "", psExit->getTerminator());
    }
  }
}

/* This function returns the basic block where instrumentation code on the
   edge from tail to head needs to be inserted. If the edge is critical, it creates
   a new basic block and returns it, else it returns either the tail or the 
   head
*/
BasicBlock* BLPPInstrumentation::splitEdge(BasicBlock *psTail, BasicBlock *psHead)
{
  if (!psHead)
  {
    assert(psTail);
    return psTail;
  }
  BasicBlock *psRelation;
  psRelation = psTail->getUniqueSuccessor();
  if (psRelation)
  {
    assert(psRelation == psHead);
    return psTail;
  }
  psRelation = psHead->getUniquePredecessor();
  if (psRelation)
  {
    assert(psRelation = psTail);
    return psHead;
  }
  BasicBlock *psNewHead = BasicBlock::Create(psTail->getContext(), "", 
    psTail->getParent());
  BranchInst::Create(psHead, psNewHead);
  replaceTarget(psTail->getTerminator(), psHead, psNewHead);
  return psNewHead;
  
}

bool BLPPInstrumentation::runOnModule(Module &m)
{
  if (NULL == psRecordEntry)
  {
    IntegerType *psFnIDType = IntegerType::get(m.getContext(), 32);
    IntegerType *psPathIDType = IntegerType::get(m.getContext(), 64);
    Type* psVoidType = Type::getVoidTy(m.getContext());
    Type* apsTypes[1] = {psFnIDType};
    ArrayRef<Type*> sRef(apsTypes, 1);
    FunctionType *psRecordEntryType = FunctionType::get
      (psVoidType, sRef, false);
    psRecordEntry = m.getOrInsertFunction("__record_entry", psRecordEntryType);
    psRecordExit = m.getOrInsertFunction("__record_exit", psRecordEntryType);
    Type *apsArgTypes[2] = {psPathIDType, psFnIDType};
    ArrayRef<Type*> sRef2(apsArgTypes, 2);
    FunctionType *psRecordPathSumType = FunctionType::get
      (psVoidType, sRef2, false);
    psRecordPathSum = m.getOrInsertFunction("__record_path_sum", psRecordPathSumType);
  }
  uint32_t i = 0;
  for (Module::iterator it = m.begin(); it != m.end(); it++)
  {
    Function &f = *it;
    if (f.isDeclaration()) continue;
    BLPP &bp=getAnalysis<BLPP>(f);
    InstrumentFunction(f, i, bp);
    i++;
  }
  return true;
}

char BLPPInstrumentation::ID = 0;
static RegisterPass<BLPPInstrumentation> X ("ppinstrument", "instrument code for BLPP profiling");


