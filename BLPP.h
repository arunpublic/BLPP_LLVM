#ifndef BLPP_H
#define BLPP_H

/* This file contains the definitions of interfaces provided by the BLPP 
   module.
   References: BLPP paper and machsuif's implementation.
   
*/
#include <list>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Dominators.h"

using namespace llvm;
typedef enum {
  PATH_SUM_INIT = 0,       // initialize path sum
  PATH_SUM_INCR,          // adjust path sum
  PATH_SUM_READ,          // use path sum to record path
  INVALID
} AnnoteType;

typedef struct {
  bool isVisited; /* For DFS's */
  unsigned int siNumPaths;
  std::list<struct TagBLPPEdge*> lOutEdges, lInEdges;
  unsigned int uiNodeID;
  void *vNodeDataP;
} BLPPNode;

struct TagBLPPEdge {
  BLPPNode *nodeHeadP, *nodeTailP;
  signed int siEdgeVal, siIncrement, siReset;
  bool isChord, isInst /* has this edge been already instrumented? */; 
  struct TagBLPPEdge* beDummyMatchP;
  AnnoteType atKind;
} ;

typedef struct TagBLPPEdge BLPPEdge;

typedef struct {
  unsigned int uiNumNodes;
  BLPPNode **bnPP;
} BLPPPath;

/* This class is more for code reuse than for anything else. */

class BLPP : public FunctionPass {

  BLPPNode **bnTempPathPP;
  typedef std::map<BasicBlock*, BLPPNode*> MapBBToBLPPNode;
  MapBBToBLPPNode mBBToBLPPNode;

  /* These are utility functions */
  void ComputeChordIncrements();
  void ChooseST();
  void DFS_ST(signed int siEvents, BLPPNode *bnCurP,
              BLPPEdge *beP);
  signed int Dir(BLPPEdge *be1P, BLPPEdge *be2P);
  void AssociateAnnotations();
  void CreateBLPPGraph(Function &f);
  BLPPNode* CreateBLPPNode(BasicBlock *psBB, uint32_t uiNodeID);
  BLPPEdge* CreateBLPPEdge(BLPPNode *psTail, BLPPNode *psHead);

 public:
  std::list<BLPPNode*>lNodes;
  std::list<BLPPEdge*>lEdges;
  BLPPNode *bnEntryP, *bnExitP;

  BLPP() : FunctionPass(ID), bnTempPathPP(nullptr) {};
  void InitDFS();
  void MarkBLPPAnnotations();
  void AssignEdgeVals(BLPPNode *bnCurrentP = nullptr);
  BLPPPath RegeneratePath(signed int siPathID);
  virtual bool runOnFunction(Function &f);
  virtual void getAnalysisUsage(AnalysisUsage &AU)
  {
  }
  virtual const char* getPassName() {return "blpp";}
  static char ID;
  ~BLPP();
  
};
#endif
