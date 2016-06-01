#ifndef BLPP_DB_H
#define BLPP_DB_H

/* This file defines the interface provided by the BLPP Database */
#include "llvm/Analysis/BLPP.h"
#include "llvm/Analysis/blpp_if.h"
#include <map>
#include <string>

#define BLPPDB_MAX_KEY_LEN        (50)

typedef struct AnnotatedPath {
	BLPPPath bPath;
	float flExecFreq;

  bool operator < (const AnnotatedPath &ap)
  {
    return (flExecFreq <= ap.flExecFreq);
  }
} AnnotatedPath;

#if 0
template <> struct hash <std::string>
{
hash () : hashstr (hash <const char *> () )
{ }
size_t operator () (const std::string & str) const
{
return hashstr (str.c_str () );
}
private:
hash <const char *> hashstr;
};
#endif

typedef struct {
	uint32_t uiTarget;
	uint32_t  uiFreq;
} EdgeFreq;


class BLPPDB : public FunctionPass {
 protected:
	FILE *fDBP; /* The database file */

	Function *psCurFunc; /* Current Function */
	unsigned int uiCurFnID;

	/* Path, Edge and Node Frequencies */
	std::map<std::string, std::list<AnnotatedPath> > ht;

	/* We could as well have moved these to the instrumentation data structures,
		 but those memory would be useless while generating code for instrumentation.
		 The price to pay is the amount of extra memory needed simultaneously for
		 a while when both bp and edge/node profile are both valid. But once 
		 edge/node/path profile information has been gathered, bp can be freed.
	*/
	uint32_t *uiNodeFrequencyP; /* Indexed by BB ID's */
  uint32_t uiFnID;
	/* A list of successors and edge frequencies, for each node */
	std::list<EdgeFreq> *lSuccessorFreqP; 

	/* Profile Header and number of functions */
	BLPPDBHdr *bhP;
	unsigned int uiNumFuncs;
	
	
 public:
  static char ID;
  BLPPDB();
	~BLPPDB();

	/* This function initializes the database.
		 Inputs:
		   fDBNameP -> The database file.
		 Return Value:
		   None
	*/
	void init(const char *fDBNameP);
  virtual bool runOnFunction(Function &f);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const
  {
    AU.addRequired<BLPP>();
  }
	/* This function returns 1 iff the input function was ever executed in
		 the profile run. Otherwise it returns 0.
		 Inputs:
		   uiFnID      -> Function ID
		 Return Value:
		   1, if the function corresponding to FunctionID got executed in the
			 profile run; 0 otherwise
		 Side Effects:
		   None
	*/

	unsigned int was_called (unsigned int uiFnID);


	/* This function sets the context for the queries, which are context
		 sensitive. The context is defined by the function id and the 
		 function's CFG representation
		 Inputs:
		   uiFnID -> ID of the function defining the context
			 cfgCurFnP -> The function's CFG representation
	*/
	void set_context(unsigned int uiFnID);

	/* This function returns the list of paths between two nodes, whose
		 combined execution frequencies cross the specified threshold.
		 Inputs:
		   uiSrcNode   -> Source Node
			 uiDestNode  -> Destination
			 flExecFreq  -> Execution Frequency Threshold
		 Return Value:
		   A List of paths 
	*/
	std::list<BLPPPath> get_hot_paths(unsigned int uiSrcNode,
																	 unsigned int uiDestNode,
																	 float flExecFreq);

	/* This function returns the number of times, the specified BB was 
		 executed. 
		 Inputs:
		   uiNode      -> BB node number
		 Return Value:
		   Number of times the basic block was executed in the profile run
		 PreConditions:
		   The last set_context call must have been for the function in which
			 the basic block is present. That context should not have been cleared.
	*/
	uint32_t get_block_frequency (unsigned int uiNode);

	/* This function returns the number of times, the specified edge was 
		 taken in the profile run.
		 Inputs:
		   uiSrcNode     -> source of the edge
			 uiTargetNode  -> target of the edge
		 Return Value:
		   Number of times the edge was taken in the profile run
		 PreConditions:
		   The last set_context call must have been for the function in which
			 the basic block is present. That context should not have been cleared.
	*/
	uint32_t get_edge_frequency(unsigned int uiSrcNode, 
															unsigned int uiTargetNode);

	/* This function cleans up the data structures created for the current context 
		 Inputs:
	     None
		 Return Value:
	     None
		 Side Effects:
		   Hash Table Contents are cleared; BLPPPath context is cleared.
	*/


	void clean_context();		 

 private:
	/* This is a helper function that sorts all paths between a source and
		 destination in the decreasing order of execution frequencies; It also
		 normalizes execution count of each path.
		 Inputs: 
		   None
		 Return Value:
		   None
		 Side effects:
		   List is sorted; Execution Count is normalized to 1, for each path
	*/
	void sort_normalize();

	/* Another helper function. Increments the edge frequency count.
		 Inputs:
		   uiSrc      -> Source node of the edge
			 uiTarget   -> Target node 
			 uiCount    -> By how much to increment
		 Return Value:
		   None
		 Side Effects:
		   Increments the edge frequency of the edge (lSuccessorFreqP)
	*/
	void increment_edge_frequency(unsigned int uiSrc, unsigned int uiTarget,
																unsigned int uiCount);
};

/* This function normalizes the execution path count for a list of paths.
	 Inputs:
	   l -> Annotated Path List
	 Return Values:
	   None
	 Side Effect:
	   Execution Count of each path in the list is normalized.
*/

void normalize(std::list<AnnotatedPath> &l);


/* This function sorts the list of paths in decreasing order of execution 
	 frequency.
	 Inputs:
	   l -> Annotated Path List
	 Return Values:
	   None
	 Side Effect:
	   The list is sorted.
*/
void sort(std::list<AnnotatedPath>& l);

#endif
