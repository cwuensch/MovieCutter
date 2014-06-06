/*
 *   Copyright (c) International Business Machines Corp., 2000-2002
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include "config.h"
#include "jfs_types.h"
#include "jfs_dmap.h"
#include "diskmap.h"

/*
 * budtab[]
 *
 * used to determine the maximum free string in a character
 * of a wmap word.  the actual bit values of the character
 * serve as the index into this array and the value of the
 * array at that index is the max free string.
 *
 */
static int8_t budtab[256] = {
	3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1
};

/*
 * NAME: ujfs_maxbuddy
 *
 * FUNCTION: Determines the maximum string of free blocks within a word of the
 *	wmap or pmap consistent with binary buddy.
 *
 * PRE CONDITIONS:
 *
 * POST CONDITIONS:
 *
 * PARAMETERS:
 *	cp	- Pointer to wmap or pmap word.
 *
 * NOTES:
 *
 * DATA STRUCTURES:
 *
 * RETURNS: Maximum string of free blocks within word.
 */
int8_t ujfs_maxbuddy(unsigned char *cp)
{
	/*
	 * Check if the wmap or pmap word is all free. If so, the free buddy size is
	 * BUDMIN.
	 */
	if (*((uint32_t *) cp) == 0) {
		return (BUDMIN);
	}

	/*
	 * Check if the wmap or pmap word is half free. If so, the free buddy size
	 * is BUDMIN-1.
	 */
	if (*((uint16_t *) cp) == 0 || *((uint16_t *) cp + 1) == 0) {
		return (BUDMIN - 1);
	}

	/*
	 * Not all free or half free. Determine the free buddy size through table
	 * lookup using quarters of the wmap or pmap word.
	 */
	return (MAX(MAX(budtab[*cp], budtab[*(cp + 1)]),
		    MAX(budtab[*(cp + 2)], budtab[*(cp + 3)])));
}

/*
 * NAME: ujfs_adjtree
 *
 * FUNCTION: Calculate the tree of a dmap or dmapctl.
 *
 * PRE CONDITIONS:
 *
 * POST CONDITIONS:
 *
 * PARAMETERS:
 *	cp	- Pointer to the top of the tree
 *	l2leaves- Number of leaf nodes as a power of 2
 *	l2min	- Number of disk blocks actually covered by a leaf of the tree;
 *		  specified as a power of 2
 *
 * NOTES: This routine first works against the leaves of the tree to calculate
 *	the maximum free string for leaf buddys.  Once this is accomplished the
 *	values of the leaf nodes are bubbled up the tree.
 *
 * DATA STRUCTURES:
 *
 * RETURNS:
 */
int8_t ujfs_adjtree(int8_t * treep, int32_t l2leaves, int32_t l2min)
{
	int32_t nleaves, leaf_index, l2max, nextb, bsize, index;
	int32_t l2free, leaf, num_this_level, nextp;
	int8_t *cp0, *cp1, *cp = treep;

	/*
	 * Determine the number of leaves of the tree and the
	 * index of the first leaf.
	 * Note: I don't know why the leaf_index calculation works, but it does.
	 */
	nleaves = (1 << l2leaves);
	leaf_index = (nleaves - 1) / 3;

	/*
	 * Determine the maximum free string possible for the leaves.
	 */
	l2max = l2min + l2leaves;

	/*
	 * Try to combine buddies starting with a buddy size of 1 (i.e. two leaves).
	 * At a buddy size of 1 two buddy leaves can be combined if both buddies
	 * have a maximum free of l2min; the combination will result in the
	 * left-most buddy leaf having a maximum free of l2min+1.  After processing
	 * all buddies for a certain size, process buddies at the next higher buddy
	 * size (i.e. current size * 2) and the next maximum free
	 * (current free + 1).  This continues until the maximum possible buddy
	 * combination yields maximum free.
	 */
	for (l2free = l2min, bsize = 1; l2free < l2max; l2free++, bsize = nextb) {
		nextb = bsize << 1;

		for (cp0 = cp + leaf_index, index = 0; index < nleaves;
		     index += nextb, cp0 += nextb) {
			if (*cp0 == l2free && *(cp0 + bsize) == l2free) {
				*cp0 = l2free + 1;
				*(cp0 + bsize) = -1;
			}
		}
	}

	/*
	 * With the leaves reflecting maximum free values bubble this information up
	 * the tree.  Starting at the leaf node level, the four nodes described by
	 * the higher level parent node are compared for a maximum free and this
	 * maximum becomes the value of the parent node.  All lower level nodes are
	 * processed in this fashion then we move up to the next level (parent
	 * becomes a lower level node) and continue the process for that level.
	 */
	for (leaf = leaf_index, num_this_level = nleaves >> 2; num_this_level > 0;
	     num_this_level >>= 2, leaf = nextp) {
		nextp = (leaf - 1) >> 2;

		/*
		 * Process all lower level nodes at this level setting the value of the
		 * parent node as the maximum of the four lower level nodes.
		 */
		for (cp0 = cp + leaf, cp1 = cp + nextp, index = 0;
		     index < num_this_level; index++, cp0 += 4, cp1++) {
			*cp1 = TREEMAX(cp0);
		}
	}

	return (*cp);
}

/*
 * NAME: ujfs_getagl2size
 *
 * FUNCTION: Determine log2(allocation group size) based on size of aggregate
 *
 * PARAMETERS:
 *	size		- Number of blocks in aggregate
 *	aggr_block_size	- Aggregate block size
 *
 * RETURNS: log2(allocation group size) in aggregate blocks
 */
int32_t ujfs_getagl2size(int64_t size, int32_t aggr_block_size)
{
	int64_t sz;
	int64_t m;
	int32_t l2sz;

	(void)aggr_block_size;

	if (size < BPERDMAP * MAXAG) {
		return (L2BPERDMAP);
	}

	m = ((uint64_t) 1 << (64 - 1));
	for (l2sz = 64; l2sz >= 0; l2sz--, m >>= 1) {
		if (m & size) {
			break;
		}
	}

	sz = (int64_t) 1 << l2sz;
	if (sz < size) {
		l2sz += 1;
	}

	return (l2sz - L2MAXAG);
}
