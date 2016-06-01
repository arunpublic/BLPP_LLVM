#ifndef BLPP_IF_H
#define BLPP_IF_H

#include <stdint.h>

typedef struct BLPPDBHdr {
	unsigned int uiFunctionID;
	uint32_t uiOffset;
	uint32_t uiNumPaths;
} BLPPDBHdr;

typedef struct BLPPProfInfo {
	uint64_t uLPathID;
	unsigned int uiExecCount;
} BLPPProfInfo;


#define BLPPDB_HDR_SIZE (sizeof(BLPPDBHdr))

#endif

