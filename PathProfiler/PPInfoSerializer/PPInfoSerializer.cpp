#include <stdio.h>
#include <assert.h>
#include <vector>
#include <hash_map>
#include "llvm/Analysis/blpp_if.h"

static int siFirstTime;
static unsigned long uiFirstProcID;
static signed int siMaxProcID;

std::vector<__gnu_cxx::hash_map<uint64_t, unsigned int> > vProcPathMaps(10);

/* This function returns the total number of recorded paths for a function.
	 Inputs:
	   hm -> Hash Map that maps path id with execution count, for the function
	 Return Value:
	   Number of recorded paths for the function.
*/
	 
static unsigned int get_total_path_count(__gnu_cxx::hash_map<uint64_t, unsigned int> hm) {
	unsigned int uiNumPaths = 0;
	for(__gnu_cxx::hash_map<uint64_t, unsigned int>::iterator it = hm.begin(); it != hm.end();
			it++) {
		uiNumPaths += (*it).second;
	}
	return uiNumPaths;
}

extern "C"
void __record_entry(unsigned long id) {

  if (!siFirstTime) {
    uiFirstProcID = id;
    siFirstTime = 1;
  }
  
}


extern "C"
void __record_exit(unsigned long id) {

	int i;
	__gnu_cxx::hash_map<uint64_t, unsigned int> h;
	BLPPDBHdr bdbh;
	BLPPProfInfo bprof;
	unsigned int uiFixedOffset, uiCumPathCount;

	if (id == uiFirstProcID) {
		FILE *fp = fopen("prof.res", "wb");
		assert(fp != NULL);
	
		/* Write all path frequencies to the file - in the format defined by
		   blpp_if.h
		*/

		/* First the header */
		uiFixedOffset = (siMaxProcID + 2) * sizeof(BLPPDBHdr);
		uiCumPathCount = 0;
		for (i = 0; i <= siMaxProcID; i++) {
			bdbh.uiFunctionID = i;
			bdbh.uiOffset = uiFixedOffset + (uiCumPathCount * sizeof(BLPPProfInfo));
			bdbh.uiNumPaths = vProcPathMaps[i].size();//get_total_path_count(vProcPathMaps[i]);
			uiCumPathCount += bdbh.uiNumPaths;
			fwrite(&bdbh, sizeof(BLPPDBHdr), 1, fp);
		}

		/* A dummy function id */
		bdbh.uiFunctionID = i;
		bdbh.uiOffset = uiFixedOffset + (uiCumPathCount * sizeof(BLPPProfInfo));
		bdbh.uiNumPaths = 0;
		fwrite(&bdbh, sizeof(BLPPDBHdr), 1, fp);
	
	 	for (i = 0; i <= siMaxProcID; i++) {
			h = vProcPathMaps[i];
			for(__gnu_cxx::hash_map<uint64_t, unsigned int>::iterator it = h.begin(); it != h.end();
				it++) {
				bprof.uLPathID = (*it).first;
				bprof.uiExecCount = (*it).second;
				fwrite(&bprof, sizeof(BLPPProfInfo), 1, fp);
			}
		}
	
		fclose(fp);
	}

}


extern "C"
void __record_path_sum(uint64_t uiPathID, signed int siProcID) {
	__gnu_cxx::hash_map<uint64_t, unsigned int>::iterator it;
	__gnu_cxx::hash_map<uint64_t, unsigned int>::value_type *vt;

	if (siProcID >= vProcPathMaps.size()) {
		vProcPathMaps.resize(siProcID * 2);
	}

	it = vProcPathMaps[siProcID].find(uiPathID); 
	if (it != vProcPathMaps[siProcID].end()) {
		(*it).second = (*it).second + 1;
	} else {
		vProcPathMaps[siProcID].insert(__gnu_cxx::hash_map<uint64_t, unsigned int>::value_type(uiPathID, 1));
	}
	if (siProcID > siMaxProcID) {
		siMaxProcID = siProcID;
	}

}
