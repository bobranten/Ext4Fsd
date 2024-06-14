#ifndef SWAP_H
#define SWAP_H

/* Definitions for a Linux swap header */

#define SWAP_HEADER_OFFSET		0
#define SWAP_HEADER_MAGIC_V1	"SWAP-SPACE"
#define SWAP_HEADER_MAGIC_V2	"SWAPSPACE2"

/* The following is a subset of linux/include/linux/swap.h */

union swap_header {
	struct 
	{
		char reserved[PAGE_SIZE - 10];
		char magic[10];
	} magic;
	struct 
	{
		char	     bootbits[1024];	/* Space for disklabel etc. */
		unsigned int version;
		unsigned int last_page;
		unsigned int nr_badpages;
		unsigned int padding[125];
		unsigned int badpages[1];
	} info;
};

#endif /* SWAP_H */
