#include "llvm/Analysis/BLPP.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include <iostream>

/* This file contains the implementation for blpp.h */
using namespace llvm;
char BLPP::ID = 0;
static RegisterPass<BLPP>
BLPPRegistration("blpp", "instrument code for BL path profiling");
/* This function marks all vertices in BLPP graph as unvisited.
   Inputs, Return Value:
     None
   SideEffects:
     As mentioned above
*/
void BLPP::InitDFS() {
  BLPPNode *nodeCurP;
  for (std::vector<BLPPNode*>::iterator it = svNodes.begin();
       it != svNodes.end(); it++) {
    nodeCurP = *it;
    nodeCurP->isVisited = false;
  }
}


/* This function assigns edge values for BLPP graph. It does this
   while leaving a node in DFS.

   Input:
     bnCurNodeP  -> Node being visited.

   Return Value:
     None

   SideEffects:
     EdgeVal field is updated for all edges

   Preconditions:
     BLPPGraph must be a DAG
*/

void BLPP::AssignEdgeVals(BLPPNode *bnCurrentP) {
  BLPPNode *bnSuccP;
  BLPPEdge *beOutP;
  signed int siNumPaths;

  if (NULL == bnCurrentP) {
    InitDFS();
    bnCurrentP = bnEntryP;
  }

  assert((false == bnCurrentP->isVisited) && "EdgeVals: Trying to visit again");
  bnCurrentP->isVisited = true;

  if (0 == (bnCurrentP->lOutEdges).size()) {
    bnCurrentP->siNumPaths = 1;
  } else {
    siNumPaths = 0;
    for (std::list<BLPPEdge*>::iterator 
           it = (bnCurrentP->lOutEdges).begin(); 
         it != (bnCurrentP->lOutEdges).end(); it++) {
      beOutP = *it;
      bnSuccP = beOutP->nodeHeadP;
      if (false == bnSuccP->isVisited) {
        AssignEdgeVals(bnSuccP);
      }
      beOutP->siEdgeVal = siNumPaths;
      siNumPaths = siNumPaths + bnSuccP->siNumPaths;
    }
    bnCurrentP->siNumPaths = siNumPaths;
  }
#if 1
  printf("Number of paths from %u to exit: %d\n", bnCurrentP->uiNodeID, 
         bnCurrentP->siNumPaths);
#endif
}

/* This function chooses a spanning tree, and computes increments for chords
   with respect to the chosen spanning tree.

   Input, Return Value:
     None

   SideEffects:
     As described above

   Preconditions:
     BLPP Graph must be cyclic (the only backedge is the exit->entry edge)
*/

void BLPP::ComputeChordIncrements() {
  BLPPEdge *beCurP;

  for (std::list<BLPPEdge*>::iterator it = lEdges.begin();
       it != lEdges.end(); it++) {
    beCurP = *it;
    if (true == beCurP->isChord) {
      beCurP->siIncrement = 0;
    }
  }

  /* Do a DFS of ST - based on Fig 4 in paper */
  DFS_ST(0, bnEntryP, NULL);

  /* Finish computing increments */
  for (std::list<BLPPEdge*>::iterator it = lEdges.begin();
       it != lEdges.end(); it++) {
    beCurP = *it;
    if (true == beCurP->isChord) {
      beCurP->siIncrement = beCurP->siIncrement + beCurP->siEdgeVal;
    }
  }
}

/* This function chooses a spanning tree for the BLPP graph. The graph is
   treated as an undirected graph!

   Input, Return Value:
     None

   SideEffects:
     isChord is set for each edge

   PreConditions:
     BLPP Graph must be cyclic (the only backedge is the exit->entry edge)
*/

void BLPP::ChooseST() {
  BLPPEdge *beCurP;
  BLPPNode *bnHeadP, *bnTailP;

  /* We will abuse isVisited field of nodes to tell if the node has been added
     to the ST or not.
  */
  InitDFS();
  bnEntryP->isVisited = true;
  bnExitP->isVisited = true;
  
  for (std::list<BLPPEdge*>::iterator it = lEdges.begin(); 
       it != lEdges.end(); it++) {
    beCurP = *it;
    bnHeadP = beCurP->nodeHeadP;
    bnTailP = beCurP->nodeTailP;
    beCurP->siIncrement = 0;

    if ((false == bnHeadP->isVisited) || (false == bnTailP->isVisited)) {
      /* Atleast one of them is not in ST. So we add this edge */
      bnHeadP->isVisited = bnTailP->isVisited = true;
      beCurP->isChord = false;
    } else if ((bnTailP == bnExitP) && (bnHeadP == bnEntryP)) {
      /* By default, everything is a tree edge */
      beCurP->isChord = false;
    } else {
      beCurP->isChord = true;
    }
  }
}


/* This function computes increments, less their edge values, for chords 
   in one pass. It actually does this by a DFS of ST.
   
   Inputs:
     siEvents       -> Edge Value for the tree edge
     bnCurP         -> Current Node being handled
     beP            -> The incoming tree edge into the source of the
                       tree edge
   
   Return Value:
     None

   SideEffects:
     Increments for chords are updated
*/

void BLPP::DFS_ST(signed int siEvents, BLPPNode *bnCurP,
                 BLPPEdge *beP) {

  BLPPEdge *beCurP;

  for (std::list<BLPPEdge*>::iterator 
         it = (bnCurP->lInEdges).begin(); 
       it != (bnCurP->lInEdges).end(); it++) {

    beCurP = *it; /* f: tgt(f) = bnCurP */
    assert((bnCurP == beCurP->nodeHeadP) && "DFS_ST: Edge, Node Inconsistency\n");

    if ((false == beCurP->isChord) && (beP != beCurP)) {
      /* f is in ST, and f is not beP */
      DFS_ST(Dir(beP, beCurP) * siEvents + beCurP->siEdgeVal, 
             beCurP->nodeTailP, beCurP);
    }
  }

  for (std::list<BLPPEdge*>::iterator
         it = (bnCurP->lOutEdges).begin(); 
       it != (bnCurP->lOutEdges).end(); it++) {

    beCurP = *it; /* f: src(f) = bnCurP */
    assert((bnCurP == beCurP->nodeTailP) && "DFS_ST: Edge, Node Inconsistency\n");

    if ((false == beCurP->isChord) && (beP != beCurP)) {
      /* f is in ST, and f is not beP */
      DFS_ST(Dir(beP, beCurP) * siEvents + beCurP->siEdgeVal,
             beCurP->nodeHeadP, beCurP);
    }
  }

  for (std::list<BLPPEdge*>::iterator it = lEdges.begin();
       it != lEdges.end(); it++) {
    beCurP = *it;
    if ((true == beCurP->isChord) /* Chord edges */ && 
        ((bnCurP == beCurP->nodeHeadP) || (bnCurP == beCurP->nodeTailP))) {
      /* incident to bnCurP */
      beCurP->siIncrement = beCurP->siIncrement + Dir(beP, beCurP) * siEvents;
    }
  }

}

/* This function returns 1 if the input edges are in the same direction,
   and -1, otherwise.

   Inputs:
     be1P, be2P -> Two edges in BLPP graph

   Return Value:
     1 or -1

   Precondition:
     The edges must share a common end point.
*/

signed int BLPP::Dir(BLPPEdge *be1P, BLPPEdge *be2P) {
  signed int siRetVal;
  if (NULL == be1P) {
    siRetVal = 1;
  } else {
    assert(((be1P->nodeHeadP == be2P->nodeTailP) || 
           (be1P->nodeTailP == be2P->nodeHeadP) ||
           (be1P->nodeHeadP == be2P->nodeHeadP) ||
           (be1P->nodeTailP == be2P->nodeTailP)) && 
          "Can't compare edge directions, because they don't share an end point");

    if ((be1P->nodeTailP == be2P->nodeHeadP) ||
        (be1P->nodeHeadP == be2P->nodeTailP)) {
      siRetVal = 1;
    } else {
      siRetVal = -1;
    }
  }
    
  return siRetVal;
}

/* This function creates a list of annotations that must be created for
   each edge of the original CFG. This list will be maintained across
   different regions, and will grow.

   Input, ReturnValue:
     None

   SideEffects:
     Entries may be added to the list of edge annotations
*/

void BLPP::AssociateAnnotations() {
  std::list<BLPPNode*> lWS;
  BLPPNode *bnCurP;
  BLPPEdge *beOutP, *beInP, *beCurP;
  lWS.push_back(bnEntryP);
  beInP = beOutP = beCurP = NULL;
  bnCurP = NULL;

  /* based on Figure 8 in PP paper */

  while (lWS.size() > 0) {
    bnCurP = lWS.back();
    lWS.pop_back();
    for (std::list<BLPPEdge*>::iterator 
           it = (bnCurP->lOutEdges).begin();
         it != (bnCurP->lOutEdges).end();
         it++) {
      beOutP = *it;
      if (beOutP->isChord) {
        /* This is a chord! Need to initialize now */
        if (NULL != beOutP->beDummyMatchP) {
          /* Move the initialization to matching dummy edge */
          (beOutP->beDummyMatchP)->siReset = beOutP->siIncrement;
          (beOutP->beDummyMatchP)->isReset = true;
          beOutP->isInst = true; /* Set to true, so we don't reconsider this edge */
          beOutP->siIncrement = 0;
          beOutP->atKind = INVALID;
        } else {
          beOutP->isInst = true; /* Non-dummy chord, initialization possible */
          beOutP->atKind = PATH_SUM_INIT;
        } 
        
      } else if (((beOutP->nodeHeadP)->lInEdges).size() == 1) {
        lWS.push_back(beOutP->nodeHeadP);
      } else {
        /* Neither a chord, nor the only incoming edge of the successor!
           So, init explicitly.
        */
        if (NULL != beOutP->beDummyMatchP)
        {
          /* Move the initialization to matching dummy edge */
          (beOutP->beDummyMatchP)->siReset = beOutP->siIncrement;
          (beOutP->beDummyMatchP)->isReset = true;
          beOutP->isInst = true; /* Set to true, so we don't reconsider this edge */
          beOutP->siIncrement = 0;
          beOutP->atKind = INVALID;
        } else {
          beOutP->isInst = true;
          beOutP->atKind = PATH_SUM_INIT;
        }
      }
      
    }
  }

  /* Code for PATH_SUM_READ */
  lWS.push_back(bnExitP);
  while (lWS.size() > 0) {

    bnCurP = lWS.back();
    lWS.pop_back();

    for (std::list<BLPPEdge*>::iterator 
           it = (bnCurP->lInEdges).begin();
         it != (bnCurP->lInEdges).end();
         it++) {

      beInP = *it;

      /* All chord edges will have increments. 
         The chords that we examine here have to increment
         the taken counts of paths. A few of them also have
         to re-initialize the sum.
      */
      if (beInP->isChord) {
        beInP->isInst = true;
        beInP->atKind = PATH_SUM_READ;
      } else if (beInP->isReset) {
        /* This is a tree edge, with a reinitialization value.
           This must be a dummy edge, or its reset would be 
           zero - any non-dummy edge does not have to reinitialize.
        */
        assert((NULL != beInP->beDummyMatchP) && "Tree, non-dummy edge "
              "needs initialization!");
        beInP->isInst = true;
        beInP->atKind = PATH_SUM_READ;
        beInP->siIncrement = 0;

      } else if (((beInP->nodeTailP)->lOutEdges).size() == 1) {
        lWS.push_back(beInP->nodeTailP);
      } else {
        /* Not a chord, No Reset Value. It just has to increment the
           path frequency.
        */
        beInP->isInst = true;
        beInP->atKind = PATH_SUM_READ;
        beInP->siIncrement = beInP->siReset = 0;
        beInP->isReset = false;
      }

    }
    
  }

  /* Register increment code for all other chords */
  for (std::list<BLPPEdge*>::iterator it = lEdges.begin();
       it != lEdges.end(); it++) {
    beCurP = *it;
    if ((false != beCurP->isChord) && (false == beCurP->isInst) &&
      (0 != beCurP->siIncrement)) {
      beCurP->isInst = true;
      beCurP->atKind = PATH_SUM_INCR;
      beCurP->siReset = 0;
    }
    if (beCurP->isInst)
    {
      std::cout << "Edge from " << beCurP->nodeTailP->uiNodeID << " to " << beCurP->nodeHeadP->uiNodeID << ";InstKind: " << beCurP->atKind << ":" << beCurP->siIncrement << ":" <<  beCurP->siReset << ":";
      if (beCurP->beDummyMatchP == NULL)
        std::cout << "Not Dummy\n";
      else
        std::cout << "Dummy\n";
    }
  }

}

/* This function marks edges with BLPP annotations.

   Inputs, Return Value:
     None

   SideEffect:
     As mentioned above.
*/
void BLPP::MarkBLPPAnnotations() {

  /* First, assign a unique path number to each path from entry to exit, and 
     assign edge values to compute path ID
  */
  AssignEdgeVals(nullptr);

  /* For efficient instrumentation: 1.Create an edge from exit to entry */
  BLPPEdge *beBackP;
  beBackP = new BLPPEdge;
  beBackP->nodeHeadP = bnEntryP;
  beBackP->nodeTailP = bnExitP;
  beBackP->beDummyMatchP = NULL; 
  beBackP->siEdgeVal = 0;
  beBackP->siReset = 0;
  beBackP->isInst = false;
  beBackP->isChord = false;
  beBackP->isReset = false;
  beBackP->siIncrement = 0;
  beBackP->atKind = INVALID;
  
  (bnExitP->lOutEdges).push_back(beBackP);
  (bnEntryP->lInEdges).push_back(beBackP);
  lEdges.push_back(beBackP);

  /* 2. spanning tree */
  ChooseST();

  /* 3. Compute chord increments wrt. chosen spanning tree */
  ComputeChordIncrements();

  /* 4. Associate Annotations */
  AssociateAnnotations();

}

/* This function returns the path corresponding to the path id.
   Inputs:
     uiPathID -> The unique ID for the path
   Return Value:
     The path, as a sequence of nodes.
*/
BLPPPath BLPP::RegeneratePath(signed int siPathID) {

  unsigned int uiPathLen = 0;
  BLPPPath ebpCorrespondingPath;
  BLPPEdge *eNextEdgeP = NULL;
  BLPPNode *bnCurrentP;

  /* Unfortunately the constructor does not need a BLPP graph.
     So I'll initialize here.
  */
  if (NULL == bnTempPathPP) {
    bnTempPathPP = new BLPPNode* [svNodes.size()];
  }

  /* Start from the initial node */
  bnCurrentP = bnEntryP;
  while (bnCurrentP != bnExitP) 
  {
    /* Of all the edges incident to the current node, choose the
       one with the greatest event number e, such that e <= uiPathID 
    */
    signed int siLargestEventNum = -1;
    eNextEdgeP = NULL;
    for (std::list<BLPPEdge*>::iterator it = 
           (bnCurrentP->lOutEdges).begin(); 
         it != (bnCurrentP->lOutEdges).end();
         it++)
    {
      BLPPEdge *eIncidentP = *it;
      if ((eIncidentP->siEdgeVal > siLargestEventNum) &&
          ((eIncidentP->siEdgeVal) <= siPathID)) {
        siLargestEventNum = eIncidentP->siEdgeVal;
        eNextEdgeP = eIncidentP;
      }
    }

    assert(NULL != eNextEdgeP && "null edge before encountering exit node\n");
    assert(eNextEdgeP->nodeTailP == bnCurrentP);
    /*claim(eNextEdgeP->nodeTailP == bnCurrentP);*/
    siPathID -= (eNextEdgeP->siEdgeVal);

    if ((NULL == eNextEdgeP->beDummyMatchP) || 
        (bnExitP == eNextEdgeP->nodeHeadP))
    {
      /* If it is not a dummy edge, or if the dummy id is from current node to 
         exit node, then the current node is also a part of the path
      */
      bnTempPathPP[uiPathLen] = bnCurrentP;
      uiPathLen++;
    }

    bnCurrentP = (eNextEdgeP->nodeHeadP);
  } 
  #if 0
  assert(NULL != eNextEdgeP && "null edge before encountering exit node\n");

  if (NULL != eNextEdgeP->beDummyMatchP) {
  {
    bnTempPathPP[uiPathLen] = bnCurrentP;
    uiPathLen++;
  }
  #endif

  ebpCorrespondingPath.uiNumNodes = uiPathLen;
  ebpCorrespondingPath.bnPP = new BLPPNode* [uiPathLen];
  memcpy(ebpCorrespondingPath.bnPP, bnTempPathPP, 
         uiPathLen * sizeof(BLPPNode*));

  return ebpCorrespondingPath;

}

BLPPNode* BLPP::CreateBLPPNode(BasicBlock *psBB, uint32_t uiNodeID)
{
  BLPPNode *psBLPPNode = new BLPPNode;
  psBLPPNode->isVisited = false;
  psBLPPNode->siNumPaths = 0;
  psBLPPNode->uiNodeID = uiNodeID;
  psBLPPNode->vNodeDataP = psBB;
  return psBLPPNode;
}

BLPPEdge* BLPP::CreateBLPPEdge(BLPPNode *psTail, BLPPNode *psHead)
{
  BLPPEdge *psEdge = new BLPPEdge;
  psEdge->nodeHeadP = psHead;
  psEdge->nodeTailP = psTail;
  psEdge->siEdgeVal = psEdge->siIncrement = psEdge->siReset = 0;
  psEdge->isChord = psEdge->isInst = psEdge->isReset = false;
  psEdge->atKind = INVALID;
  psHead->lInEdges.push_back(psEdge);
  psTail->lOutEdges.push_back(psEdge);
  psEdge->beDummyMatchP = nullptr;
  std::cout << "Edge from " << psTail->uiNodeID << " to " << psHead->uiNodeID
    << "\n";
  return psEdge;
}



void BLPP::CreateBLPPGraph(BasicBlock *psCurNode)
{
  /* Assign an id for this node */
  std::cout << "uiNodeID:" << uiNodeID <<"\n";
    psCurNode->dump();
  BLPPNode *psBLPPNode = CreateBLPPNode(psCurNode, uiNodeID++);
  mBBToBLPPNode[psCurNode] = psBLPPNode;
  svNodes.push_back(psBLPPNode);

  if (psCurNode == &(psCurFunc->getEntryBlock()))
  {
    bnEntryP = psBLPPNode;
    bnExitP = CreateBLPPNode(nullptr, uiNodeID++);
    svNodes.push_back(bnExitP);
  }

  TerminatorInst *psTermInst = psCurNode->getTerminator();
  switch (psTermInst->getOpcode())
  {
    case Instruction::Ret:
    {
      assert(0 == psTermInst->getNumSuccessors());
      BLPPEdge *psEdge = CreateBLPPEdge(mBBToBLPPNode[psCurNode], bnExitP);
      lEdges.push_back(psEdge);
      break;
    }
    case Instruction::Br:
    {
      for (uint32_t i = 0, uiNSucc = psTermInst->getNumSuccessors(); 
        i < uiNSucc; i++)
      {
        BasicBlock *psSucc = psTermInst->getSuccessor(i);
        if (mBBToBLPPNode.find(psSucc) == mBBToBLPPNode.end())
          CreateBLPPGraph(psSucc);
        if (sDT.dominates(psSucc, psCurNode))
        {
          /* Create dummy edges */
          BLPPEdge *psFromEntry = CreateBLPPEdge(bnEntryP, mBBToBLPPNode[psSucc]);
          BLPPEdge *psToExit = CreateBLPPEdge(mBBToBLPPNode[psCurNode], bnExitP);
          psFromEntry->beDummyMatchP = psToExit;
          psToExit->beDummyMatchP = psFromEntry;
          lEdges.push_back(psFromEntry);
          lEdges.push_back(psToExit);
        }
        else
        {
          BLPPEdge *psEdge = CreateBLPPEdge(mBBToBLPPNode[psCurNode], 
            mBBToBLPPNode[psSucc]);
          lEdges.push_back(psEdge);
        }
      }
      break;
    }
    default:
    {
      assert(0 && "Unhandled terminator instruction");
    }
  }
}


bool BLPP::runOnFunction(Function &func)
{
  psCurFunc = &func;
  DominatorTreeAnalysis sDTA;
  sDT = sDTA.run(func);
  uiNodeID = 0;
  CreateBLPPGraph(&(func.getEntryBlock()));
  MarkBLPPAnnotations();
  return false;
}

BLPP::~BLPP()
{
  for (std::vector<BLPPNode*>::iterator it = svNodes.begin(); it != svNodes.end();
    it++)
  {
    delete *it;
  }
  for (std::list<BLPPEdge*>::iterator it = lEdges.begin(); it != lEdges.end();
    it++)
  {
    delete *it;
  }
  if (bnTempPathPP)
    delete [] bnTempPathPP;
}
