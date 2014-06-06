/*
 *   Copyright (C) International Business Machines Corp., 2000-2004
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
#include <ctype.h>
#include <getopt.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
/* defines and includes common among the jfs_fsck modules */
#include "xfsckint.h"
#include "xchkdsk.h"
#include "fsck_message.h"	/* message text, all messages, in english */
#include "jfs_byteorder.h"
#include "jfs_unicode.h"
#include "jfs_version.h"	/* version number and date for utils */
#include "logform.h"
#include "logredo.h"
#include "message.h"
#include "super.h"
#include "utilsubs.h"

int64_t ondev_jlog_byte_offset;

/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
 *
 * superblock buffer and pointer
 *
 *    values are assigned by the xchkdsk routine
 */
struct superblock aggr_superblock;
struct superblock *sb_ptr;

/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
 *
 * fsck aggregate info structure and pointer
 *
 *    values are assigned by the xchkdsk routine
 */
struct fsck_agg_record agg_record;
struct fsck_agg_record *agg_recptr;

/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
 *
 * fsck block map info structure and pointer
 *
 *    values are assigned by the xchkdsk routine
 */
struct fsck_bmap_record bmap_record;
struct fsck_bmap_record *bmap_recptr;

/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
 *
 * For message processing
 *
 *    values are assigned by the xchkdsk routine
 */

char *Vol_Label;
char *program_name;

struct tm *fsck_DateTime = NULL;
char time_stamp[20];

/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
 *
 * For directory entry processing
 *
 */
int32_t key_len[2];
UniChar key[2][JFS_NAME_MAX];
UniChar ukey[2][JFS_NAME_MAX];

int32_t Uni_Name_len;
UniChar Uni_Name[JFS_NAME_MAX];
int32_t Str_Name_len;
char Str_Name[JFS_NAME_MAX];

/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
 *
 * Device information.
 *
 *     values are assigned when (if) the device is opened.
 */
FILE *Dev_IOPort;
uint32_t Dev_blksize;
int32_t Dev_SectorSize;
char log_device[512] = { 0 };

/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
 *
 * Unicode path strings information.
 *
 *     values are assigned when the fsck aggregate record is initialized.
 *     accessed via addresses in the fack aggregate record.
 */
UniChar uni_LSFN_NAME[11] =
    { 'L', 'O', 'S', 'T', '+', 'F', 'O', 'U', 'N', 'D' };
UniChar uni_lsfn_name[11] =
    { 'l', 'o', 's', 't', '+', 'f', 'o', 'u', 'n', 'd' };

/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
 *
 * fsckwsp error handling fields
 *
 *     values are assigned when the fsck workspace storage is
 *     allocated.
 */
int wsp_dynstg_action;
int wsp_dynstg_object;

#define INODELISTSIZE 10
uint32_t  InodeList[INODELISTSIZE];
int       InodeListSize;


/* VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
 *
 * The following are internal to this file
 *
 */
int final_processing(void);
int initial_processing(int, char **);
bool parse_parms(int, char **);
int phase0_processing(void);
int phase1_processing(void);
int validate_fs_inodes(void);
int verify_parms(void);
void fsck_usage(void);

void WriteLogX(char *s);
extern char LogString[];
#define TAP_PrintNet(...) {sprintf(LogString, __VA_ARGS__); WriteLogX(LogString);}

/* VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV */

/* exit value */
int exit_value = FSCK_OK;

/*****************************************************************************
 * NAME: main (jfs_fsck)
 *
 * FUNCTION: Entry point for jfs check/repair of aggregate
 *
 * INTERFACE:
 *            jfs_fsck <device name>
 *
 *                         [ -a ]
 *                         autocheck mode
 *                         replay the transaction log and quit fsck unless
 *                         aggregate state is dirty or log replay failed
 *
 *                         [ -f ]
 *                         force check
 *                         replay the tranaction log and force checking
 *
 *                         [ -j journal_device ]
 *                         specify the external journal device
 *
 *                         [ -n ]
 *                         read only check
 *                         report but do not repair problems
 *
 *                         [ --omit_journal_replay ]
 *                         omit replay of the transaction log
 *
 *                         [ -p ]
 *                         preen
 *                         same functionality as -a
 *
 *                         [ --replay_journal_only ]
 *                         only replay the transaction log
 *
 *                         [ -v ]
 *                         verbose messaging
 *
 *                         [ -V ]
 *                         version information
 *                         print version information and exit
 *
 * RETURNS:
 *      success:                   FSCK_OK (0)
 *      log successfully replayed: FSCK_CORRECTED (1)
 *      errors corrected:          FSCK_CORRECTED (1)
 *      errors uncorrected:        FSCK_ERRORS_UNCORRECTED (4)
 *      operational error:         FSCK_OP_ERROR (8)
 *      usage error:               FSCK_USAGE_ERROR (16)
 */
int xchkdsk_main(int argc, char **argv)
{

	int rc = FSCK_OK;
	time_t Current_Time;

	/*
	 * some basic initializations
	 */
	sb_ptr = &aggr_superblock;
	agg_recptr = &agg_record;
	bmap_recptr = &bmap_record;
	InodeListSize = 0;

	if (argc && **argv)
		program_name = *argv;
	else
		program_name = "jfs_fsck";

	TAP_PrintNet("jfs_fsck version %s, %s", VERSION, JFSUTILS_DATE);

	wsp_dynstg_action = dynstg_unknown;
	wsp_dynstg_object = dynstg_unknown;

	/* init workspace aggregate record
	 * (the parms will be recorded in it)
	 */
	rc = init_agg_record();

	/*
	 * Allocate the multi-purpose buffer now so that it can be
	 * used during superblock verification.
	 *
	 * This must be done at least before calling logredo to ensure
	 * that the malloc() will succeed.
	 * (In autocheck mode, logredo is likely to eat up all the
	 * low memory.  We don't want to use the alloc_wrksp routine
	 * because we want a page boundary without having to burn
	 * 4096 extra bytes.
	 */
	if ((rc = alloc_vlarge_buffer()) != FSCK_OK)
	{
		/* alloc_vlarge_buffer not OK */
		exit_value = FSCK_OP_ERROR;
		goto main_exit;
	}

	if ((rc = initial_processing(argc, argv)) != FSCK_OK)
	{
		/*
		 * Something very wrong has happened.  We're not
		 * even sure we're checking a JFS file system!
		 * Appropriate messages should already be logged.
		 */
		/* initial_processing sets exit value if unsuccessful */
		goto main_exit;
	}

#ifdef CLEARBADBLOCK
	/*
	 * If they specified Clear Bad Blocks List only (aka /B),
	 * release everything that's allocated, close everything
	 * that's open, and then initiate the requested processing.
	 */
	if ((agg_recptr->parm_options[UFS_CHKDSK_CLRBDBLKLST])
	    && (!agg_recptr->fsck_is_done)) {
		/* bad block list processing only */
		/*
		 * this path is taken only when -f not specified, so
		 * fsck processing is readonly, but the clrbblks
		 * processing requires fsck to do some things it only
		 * permits when processing readwrite.  So we reset the
		 * switches temporarily and take care what routines we call.
		 */
		agg_recptr->processing_readwrite = 1;
		agg_recptr->processing_readonly = 0;
		/*
		 * JFS Clear Bad Blocks List processing
		 *
		 * If things go well, this will issue messages and
		 * write to the service log.
		 */
		rc = establish_wsp_block_map_ctl();

		/*
		 * terminate fsck service logging
		 */
		fscklog_end();
		/*
		 * restore the original values.
		 */
		agg_recptr->processing_readwrite = 0;
		agg_recptr->processing_readonly = 1;
		/*
		 * release any workspace that has been allocated
		 */
		workspace_release();
		/*
		 * Close (Unlock) the device
		 */
		if (agg_recptr->device_is_open) {
			close_volume();
		}
		/*
		 * Then exit
		 */

		return (rc);
	}
#endif

	if (agg_recptr->fsck_is_done)
		goto phases_complete;

	rc = phase0_processing();
	if (agg_recptr->fsck_is_done)
		goto phases_complete;

	/*
	 * If -n flag was specified, disable write processing now
	 */
	if (agg_recptr->parm_options[UFS_CHKDSK_LEVEL0])
	{
		agg_recptr->processing_readonly = 1;
		agg_recptr->processing_readwrite = 0;
	}

	rc = phase1_processing();

	phases_complete:
	if (!agg_recptr->superblk_ok)
	{
		/* superblock is bad */
		exit_value = FSCK_ERRORS_UNCORRECTED;
		goto close_vol;
	}

	/* we at least have a superblock */
	if ((rc == FSCK_OK) && (!(agg_recptr->fsck_is_done))) {
		/* not fleeing an error and not making a speedy exit */

		/* finish up and display some information */
		rc = final_processing();

		/* flush the I/O buffers to complete any pending writes */
		if (rc == FSCK_OK) {
			rc = blkmap_flush();
		} else {
			blkmap_flush();
		}

		if (rc == FSCK_OK) {
			rc = blktbl_dmaps_flush();
		} else {
			blktbl_dmaps_flush();
		}

		if (rc == FSCK_OK) {
			rc = blktbl_Ln_pages_flush();
		} else {
			blktbl_Ln_pages_flush();
		}

		if (rc == FSCK_OK) {
			rc = iags_flush();
		} else {
			iags_flush();
		}

		if (rc == FSCK_OK) {
			rc = inodes_flush();
		} else {
			inodes_flush();
		}

		if (rc == FSCK_OK) {
			rc = mapctl_flush();
		} else {
			mapctl_flush();
		}

		if (rc == FSCK_OK) {
			rc = flush_index_pages();
		} else {
			flush_index_pages();
		}
	}
	/*
	 * last chance to write to the wsp block map control page...
	 */
	Current_Time = time(NULL);
	fsck_DateTime = localtime(&Current_Time);

	sprintf(time_stamp, "%d/%d/%d %d:%02d:%02d",
		fsck_DateTime->tm_mon + 1,
		fsck_DateTime->tm_mday, (fsck_DateTime->tm_year + 1900),
		fsck_DateTime->tm_hour, fsck_DateTime->tm_min,
		fsck_DateTime->tm_sec);

	if (agg_recptr->processing_readwrite) {
		/* on-device fsck workspace block map */
		if (agg_recptr->blkmp_ctlptr != NULL) {
			memcpy(&agg_recptr->blkmp_ctlptr->hdr.end_time[0],
			       &time_stamp[0], 20);
			agg_recptr->blkmp_ctlptr->hdr.return_code = rc;
			blkmap_put_ctl_page(agg_recptr->blkmp_ctlptr);
		}
	}
	if (rc == FSCK_OK) {
		/* either all ok or nothing fatal */
		if (agg_recptr->processing_readonly) {
			/* remind the caller not to take
			 * any messages issued too seriously
			 */
			fsck_send_msg(fsck_READONLY);
			if (agg_recptr->corrections_needed ||
			    agg_recptr->corrections_approved) {
				fsck_send_msg(fsck_ERRORSDETECTED);
				exit_value = FSCK_ERRORS_UNCORRECTED;
			}
		}
		/* may write to superblocks again */
		rc = agg_clean_or_dirty();
	}

	if (agg_recptr->ag_modified) {
		/* wrote to it at least once */
		fsck_send_msg(fsck_MODIFIED);
	}
	if (agg_recptr->ag_dirty) {
		exit_value = FSCK_ERRORS_UNCORRECTED;
	}

	/*
	 * Log fsck exit
	 */
	fsck_send_msg(fsck_SESSEND, time_stamp, rc, exit_value);

	/*
	 * terminate fsck service logging
	 */
	fscklog_end();
        /*
	 * release all workspace that has been allocated
	 */
	if (rc == FSCK_OK) {
		rc = workspace_release();
	} else {
		workspace_release();
	}

      close_vol:
	/*
	 * Close (Unlock) the device
	 */
	if (agg_recptr->device_is_open) {
		if (rc == FSCK_OK) {
			rc = close_volume();
		} else {
			close_volume();
		}
	}

  main_exit:
	return (exit_value);
}

/***************************************************************************
 * NAME: final_processing
 *
 * FUNCTION:  If processing read/write, replicate the superblock and the
 *            aggregate inode structures (i.e., the Aggregate Inode Map
 *            and the Aggregate Inode Table).
 *
 *            Notify the user about various things.
 *
 * PARAMETERS:  none
 *
 * RETURNS:
 *      success: FSCK_OK
 *      failure: something else
 */
int final_processing()
{
	int pf_rc = FSCK_OK;

	if (agg_recptr->processing_readwrite)
	{
		/*
		 * Make sure the s_logdev is up to date in the superblock.
		 */
		if ((Log.location & OUTLINELOG) && Log.devnum)
			sb_ptr->s_logdev = Log.devnum;

		/* refresh the redundancy of the
		 * aggregate superblock (and verify
		 * successful write to the one we
		 * haven't been using)
		 */
		pf_rc = replicate_superblock();
	}

	if (pf_rc != FSCK_OK)
	{
		agg_recptr->fsck_is_done = 1;
	}

	return (pf_rc);
}

/*****************************************************************************
 * NAME: initial_processing
 *
 * FUNCTION: Parse and verify invocation parameters.  Open the device and
 *           verify that it contains a JFS file system.  Check and repair
 *           the superblock.  Initialize the fsck aggregate record.  Refresh
 *           the boot sector on the volume.  Issue some opening messages.
 *
 *
 * PARAMETERS:  as specified to main()
 *
 * NOTES: sets exit_value if other than OK
 *
 * RETURNS:
 *      success: FSCK_OK
 *      failure: something else
 */
int initial_processing(int argc, char **argv)
{
	int pi_rc = FSCK_OK;
	int iml_rc = FSCK_OK;
	int64_t fsblk_offset_bak;
	int64_t byte_offset_bak;
	time_t Current_Time;
	char message_parm[MAXPARMLEN];

	/*
	 * Initiate fsck service logging
	 */
	iml_rc = fscklog_start();

	/*
	 * Log the beginning of the fsck session
	 */
	Current_Time = time(NULL);
	fsck_DateTime = localtime(&Current_Time);
	sprintf(message_parm, "%d/%d/%d %d:%02d:%02d",
		fsck_DateTime->tm_mon + 1, fsck_DateTime->tm_mday,
		(fsck_DateTime->tm_year + 1900), fsck_DateTime->tm_hour,
		fsck_DateTime->tm_min, fsck_DateTime->tm_sec);
	fsck_send_msg(fsck_SESSSTART, message_parm);

	/*
	 * Process the parameters given by the user
	 */

	/* parse the parms and record them in the aggregate wsp record */
	if(!parse_parms(argc, argv))
	  return -1;

	if ((pi_rc = Is_Device_Mounted(Vol_Label)) != FSCK_OK)
	{
		switch (pi_rc)
		{
		case MSG_JFS_VOLUME_IS_MOUNTED_RO:
			/* is mounted read only */
			if ((agg_recptr->parm_options[UFS_CHKDSK_DEBUG]) || (agg_recptr->parm_options[UFS_CHKDSK_VERBOSE]))
			  TAP_PrintNet("FSCK  Device %s is currently mounted READ ONLY.\n", Vol_Label);
			break;

		case MSG_JFS_MNT_LIST_ERROR:
			/* setmntent failed */
			if ((agg_recptr->parm_options[UFS_CHKDSK_DEBUG]) || (agg_recptr->parm_options[UFS_CHKDSK_VERBOSE]))
			  TAP_PrintNet("jfs_fsck cannot access file system "
				       "description file to determine mount "
				       "status and file system type of device "
				       "%s.  jfs_fsck will continue. ", Vol_Label);
			break;

		case MSG_JFS_VOLUME_IS_MOUNTED:
			if (agg_recptr->parm_options[UFS_CHKDSK_LEVEL0])
			{
				/* read only */
				fsck_send_msg(fsck_FSMNTD);
				fsck_send_msg(fsck_MNTFSYS2);
			}
			else
			{
				/* is mounted */
			  TAP_PrintNet("%s is mounted.\n\nWARNING!!!\n"
				             "Running fsck on a mounted file system\n"
				             "may cause SEVERE file system damage."
				             "\n\n", Vol_Label);
			}
			break;

		case MSG_JFS_NOT_JFS:
			/* is not JFS */
		  TAP_PrintNet("%s is mounted and the file system is not type"
			             " JFS.\n\nWARNING!!!\nRunning jfs_fsck on a "
      			       "mounted file system\nor on a file system other "
			             "than JFS\nmay cause SEVERE file system damage."
			             "\n\n", Vol_Label);
			return -1;
			break;

		default:
			return -1;
			break;
		}
	}

	/* the parms are good */
	pi_rc = verify_parms();

	/*
	 * Open the device and verify that it contains a valid JFS aggregate
	 * If it does, check/repair the superblock.
	 */
	if (pi_rc != FSCK_OK)
	{
		/* verify_parms returned bad rc */
		exit_value = FSCK_USAGE_ERROR;
		goto ip_exit;
	}

	/* parms are good */
	fsck_send_msg(fsck_DRIVEID, Vol_Label);
	pi_rc = open_volume(Vol_Label);
	if (pi_rc != FSCK_OK)
	{
		/* device open failed */
		fsck_send_msg(fsck_CNTRESUPB);
		exit_value = FSCK_OP_ERROR;
		goto ip_exit;
	}

	/* device is open */
	agg_recptr->device_is_open = 1;
	pi_rc = validate_repair_superblock();
	if (pi_rc != FSCK_OK)
	{
		/* superblock invalid */
	       	exit_value = FSCK_OP_ERROR;
		goto ip_exit;
	}

	fsck_send_msg(fsck_DRIVETYPE);
	/*
	 * add some stuff to the agg_record which is based on
	 * superblock fields
	 */
	agg_recptr->log2_blksize = log2shift(sb_ptr->s_bsize);
	agg_recptr->blksperpg = BYTESPERPAGE / sb_ptr->s_bsize;
	agg_recptr->log2_blksperpg = log2shift(agg_recptr->blksperpg);
	agg_recptr->log2_blksperag = log2shift(sb_ptr->s_agsize);
	/*highest is the last one before the in-aggregate journal log */
	agg_recptr->highest_valid_fset_datablk =
	    addressPXD(&(sb_ptr->s_fsckpxd)) - 1;
	/* lowest is the first after the secondary aggreg inode table */
	agg_recptr->lowest_valid_fset_datablk =  addressPXD(&(sb_ptr->s_ait2)) +
	    (INODE_EXTENT_SIZE / sb_ptr->s_bsize) + 1;
	/*
	 *  agg size in logical blks is
	 *    (size in phys blks times phys blk size divided by log blk size)
	 *  number of AGs in the aggregate:
	 *     (agg size in log blks plus AG size in log blks minus 1)
	 *     divided by (AG size in log blks)
	 */
	agg_recptr->num_ag = ((sb_ptr->s_size * sb_ptr->s_pbsize /
	      sb_ptr->s_bsize) + sb_ptr->s_agsize - 1) / sb_ptr->s_agsize;
	agg_recptr->sb_agg_fsblk_length = sb_ptr->s_size *
	    sb_ptr->s_pbsize / sb_ptr->s_bsize;
	/* length of the on-device journal log */
	agg_recptr->ondev_jlog_fsblk_length = lengthPXD(&(sb_ptr->s_logpxd));
	/* aggregate block offset of the on-device journal log */
	agg_recptr->ondev_jlog_fsblk_offset = addressPXD(&(sb_ptr->s_logpxd));
	ondev_jlog_byte_offset =
	    agg_recptr->ondev_jlog_fsblk_offset * sb_ptr->s_bsize;
	/* length of the on-device fsck service log */
	agg_recptr->ondev_fscklog_byte_length =
	    sb_ptr->s_fsckloglen * sb_ptr->s_bsize;
	/* length of the on-device fsck service log */
	agg_recptr->ondev_fscklog_fsblk_length = sb_ptr->s_fsckloglen;
	/* length of the on-device fsck workspace */
	agg_recptr->ondev_wsp_fsblk_length = lengthPXD(&(sb_ptr->s_fsckpxd)) -
	    agg_recptr->ondev_fscklog_fsblk_length;
	/* length of the on-device fsck workspace */
	agg_recptr->ondev_wsp_byte_length =
	    agg_recptr->ondev_wsp_fsblk_length * sb_ptr->s_bsize;
	/* aggregate block offset of the on-device fsck workspace */
	agg_recptr->ondev_wsp_fsblk_offset = addressPXD(&(sb_ptr->s_fsckpxd));
	/* byte offset of the on-device fsck workspace */
	agg_recptr->ondev_wsp_byte_offset =
	    agg_recptr->ondev_wsp_fsblk_offset * sb_ptr->s_bsize;
	/* aggregate block offset of the on-device fsck workspace */
	agg_recptr->ondev_fscklog_fsblk_offset =
	    agg_recptr->ondev_wsp_fsblk_offset +
	    agg_recptr->ondev_wsp_fsblk_length;
	/* byte offset of the on-device fsck workspace */
	agg_recptr->ondev_fscklog_byte_offset =
	    agg_recptr->ondev_wsp_byte_offset +
	    agg_recptr->ondev_wsp_byte_length;
	/*
	 * The offsets now assume the prior log (the one to overwrite) is
	 * 1st in the aggregate fsck service log space.  Adjust if needed.
	 */
	if (sb_ptr->s_fscklog == 0) {
		/* first time ever for this aggregate */
		fsblk_offset_bak = agg_recptr->ondev_fscklog_fsblk_offset;
		byte_offset_bak = agg_recptr->ondev_fscklog_byte_offset;
		/*
		 * initialize the 2nd service log space
		 *
		 * (we'll actually write the log to the 1st space, so
		 * we'll initialize it below)
		 */
		agg_recptr->ondev_fscklog_fsblk_offset +=
		    agg_recptr->ondev_fscklog_fsblk_length / 2;
		agg_recptr->ondev_fscklog_byte_offset +=
		    agg_recptr->ondev_fscklog_byte_length / 2;
		agg_recptr->fscklog_agg_offset =
		    agg_recptr->ondev_fscklog_byte_offset;
		fscklog_init();
		sb_ptr->s_fscklog = 1;
		agg_recptr->ondev_fscklog_fsblk_offset = fsblk_offset_bak;
		agg_recptr->ondev_fscklog_byte_offset = byte_offset_bak;
	} else if (sb_ptr->s_fscklog == 1) {
		/* the 1st is most recent */
		sb_ptr->s_fscklog = 2;
		agg_recptr->ondev_fscklog_fsblk_offset +=
		    agg_recptr->ondev_fscklog_fsblk_length / 2;
		agg_recptr->ondev_fscklog_byte_offset +=
		    agg_recptr->ondev_fscklog_byte_length / 2;
	} else {
		/* the 1st is the one to overwrite */
		sb_ptr->s_fscklog = 1;
	}
	agg_recptr->fscklog_agg_offset = agg_recptr->ondev_fscklog_byte_offset;
	/*
	 * Initialize the service log
	 */
	fscklog_init();
	/* from the user's perspective, these are in use (by jfs) */
	agg_recptr->blocks_used_in_aggregate =
	    agg_recptr->ondev_wsp_fsblk_length +
	    agg_recptr->ondev_fscklog_fsblk_length +
	    agg_recptr->ondev_jlog_fsblk_length;
	agg_recptr->superblk_ok = 1;
	if ((!agg_recptr->parm_options[UFS_CHKDSK_LEVEL0]) &&
	    (agg_recptr->processing_readonly)) {
		/* user did not specify check only but we can only
		 * do check because we don't have write access
		 */
		fsck_send_msg(fsck_WRSUP);
	}

      ip_exit:
	return (pi_rc);
}

/*****************************************************************************
 * NAME: parse_parms
 *
 * FUNCTION:  Parse the invocation parameters.  If any unrecognized
 *            parameters are detected, or if any required parameter is
 *            omitted, issue a message and exit.
 *
 * PARAMETERS:  as specified to main()
 *
 * RETURNS:  If there is an error in parse_parms, it calls fsck_usage()
 *           to remind the user of command format and proper options.
 *           fsck_usage then exits with exit code FSCK_USAGE_ERROR.
 */

/* There may be no exit wihitn a TAP or the pvr process will exit and the Toppy reboots */
bool parse_parms(int argc, char **argv)
{
  char *device_name = NULL;
  int c;
  FILE *file_p = NULL;

  char *short_opts = "adfi:j:noprvVy";

  struct option long_opts[] = {
      { "omit_journal_replay", no_argument, NULL, 'o'},
      { "replay_journal_only", no_argument, NULL, 'J'},
      { NULL, 0, NULL, 0} };

  optind = 1;
  opterr = 0;
  optarg = NULL;
  optopt = 0;

  while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != EOF)
  {
    switch (c)
    {
      case 'r':
        /*************************
         * interactive autocheck *
         *************************/
        /*
         * jfs_fsck does not support interactive checking,
         * so -r is undocumented.  However, for backwards
         * compatibility, -r is supported here and functions
         * similarly as -p.
         */
      case 'a':
      case 'p':
        /*******************
         * preen autocheck *
         *******************/
        agg_recptr->parm_options[UFS_CHKDSK_LEVEL2] = -1;
        agg_recptr->parm_options[UFS_CHKDSK_IFDIRTY] = -1;
        break;

#if 0
      case 'b':
        /*******************************************
         * Clear LVM Bad Block List utility option *
         *******************************************/
        agg_recptr->parm_options[UFS_CHKDSK_CLRBDBLKLST] = -1;
        break;

      case 'c':
        /******************
         * IfDirty option *
         ******************/
        agg_recptr->parm_options[UFS_CHKDSK_IFDIRTY] = -1;
        break;
#endif

      case 'f':
        /********************************
         * Force check after log replay *
         ********************************/
        agg_recptr->parm_options[UFS_CHKDSK_LEVEL3] = -1;
        break;

      case 'i':
        /**************************
         * Specify inode          *
         **************************/
        if(InodeListSize < INODELISTSIZE)
        {
          InodeList[InodeListSize] = strtol(optarg, NULL, 10);
          InodeListSize++;
        }
        break;

      case 'j':
        /**************************
         * Specify journal device *
         **************************/
        strncpy(log_device, optarg, sizeof (log_device) - 1);
        break;

      case 'J':
        /***********************
         * Replay journal only *
         ***********************/
        agg_recptr->parm_options_logredo_only = 1;
        break;

      case 'n':
        /***********************************
         * Level0 (no write access) option *
         ***********************************/
        agg_recptr->parm_options[UFS_CHKDSK_LEVEL0] = -1;
        break;

      case 'o':
        /************************************
         * Omit logredo() processing option *
         ************************************/
        agg_recptr->parm_options[UFS_CHKDSK_SKIPLOGREDO] = -1;
        agg_recptr->parm_options_nologredo = 1;
        break;

      case 'd':
        /****************
         * Debug option *
         ****************/
        /* undocumented at this point, it is similar to -v */
        dbg_output = 1;
        /* no break */

      case 'v':
        /******************
         * Verbose option *
         ******************/
        agg_recptr->parm_options[UFS_CHKDSK_VERBOSE] = -1;
        break;

      case 'V':
        /**********************
         * print version only *
         **********************/
        return false;
        break;

      case 'y':
        /******************************
         * 'yes to everything' option *
         ******************************/
        /*
         * jfs_fsck does not support interactive checking,
         * so the -y option isn't necessary here.  However,
         * in striving to have options similar to those
         * of e2fsck, we will let -y be the same as the
         * default -p (unless it is overridden), since
         * the functionality is similar for both -y and -p.
         */
        break;

      default:
        fsck_usage();
        return false;
    }
  }

  if (agg_recptr->parm_options_logredo_only && (agg_recptr->parm_options_nologredo || agg_recptr->parm_options[UFS_CHKDSK_LEVEL3] || agg_recptr->parm_options[UFS_CHKDSK_LEVEL0]) )
  {
    TAP_PrintNet("Error: --replay_journal_only cannot be used with -f, -n, or --omit_journal_replay.");
    fsck_usage();
    return false;
  }

	if (optind != argc - 1)
	{
	  TAP_PrintNet("Error: Device not specified or command format error");
		fsck_usage();
		return false;
	}

	device_name = argv[optind];

	file_p = fopen(device_name, "r");
	if (file_p)
	{
		fclose(file_p);
	}
	else
	{
	  TAP_PrintNet("Error: Cannot open device %s", device_name);
		fsck_usage();
		return false;
	}

	Vol_Label = device_name;

	return true;
}

/*****************************************************************************
 * NAME: phase0_processing
 *
 * FUNCTION:  Log Redo processing.
 *
 * PARAMETERS:  none
 *
 * RETURNS:
 *      success: FSCK_OK
 *      failure: something else
 */
int phase0_processing()
{
	int p0_rc = FSCK_OK;
	int64_t agg_blks;
	int32_t use_2ndary_superblock = 0;

	agg_recptr->logredo_rc = FSCK_OK;
	/*
	 * if this flag is set then the primary superblock is
	 * corrupt.  The secondary superblock is good, but we
	 * weren't able to fix the primary version.  logredo can
	 * run, but must use the secondary version of the
	 * aggregate superblock
	 */
	if (agg_recptr->cant_write_primary_sb == 1) {
		use_2ndary_superblock = -1;
	}

	/*
	 * start off phase 0
	 */
	fsck_send_msg(fsck_FSSMMRY1, sb_ptr->s_bsize);
	/* aggregate size in fs blocks, by the user's point of view. */
	agg_blks =
	    agg_recptr->sb_agg_fsblk_length +
	    agg_recptr->ondev_jlog_fsblk_length +
	    agg_recptr->ondev_fscklog_fsblk_length +
	    agg_recptr->ondev_wsp_fsblk_length;
	fsck_send_msg(fsck_FSSMMRY2, (long long) agg_blks);
	/*
	 * logredo processing
	 */
	if ((agg_recptr->processing_readwrite) &&
	    (!agg_recptr->parm_options_nologredo)) {
		/* read/write access AND user didn't say not to need to invoke logredo */
		fsck_send_msg(fsck_PHASE0);

		/*
		 * write the superblock to commit any changes we have made in it
		 */
		if (use_2ndary_superblock) {
			/* put 2ndary */
			ujfs_put_superblk(Dev_IOPort, sb_ptr, 0);
		} else {
			/* put primary  */
			ujfs_put_superblk(Dev_IOPort, sb_ptr, 1);
		}

		agg_recptr->logredo_rc =
		    jfs_logredo(Vol_Label, Dev_IOPort, use_2ndary_superblock);

		/* If superblock is clean and log in use, it's okay */
		if ((agg_recptr->logredo_rc == LOG_IN_USE) &&
		    (sb_ptr->s_state == FM_CLEAN))
			agg_recptr->logredo_rc = FSCK_OK;

		if (agg_recptr->logredo_rc != FSCK_OK) {
			/* logredo failed */
			fsck_send_msg(fsck_LOGREDOFAIL, agg_recptr->logredo_rc);
		} else {
			fsck_send_msg(fsck_LOGREDORC, agg_recptr->logredo_rc);
		}
		/*
		 * logredo may change the superblock, so read it in again
		 */
		if (use_2ndary_superblock) {
			/* get 2ndary */
			ujfs_get_superblk(Dev_IOPort, sb_ptr, 0);
		} else {
			/* get primary  */
			ujfs_get_superblk(Dev_IOPort, sb_ptr, 1);
		}
	}
	if (agg_recptr->parm_options[UFS_CHKDSK_IFDIRTY] &&
	    (!agg_recptr->parm_options_nologredo) &&
	    ((sb_ptr->s_state & FM_DIRTY) != FM_DIRTY)
	     && (agg_recptr->logredo_rc == FSCK_OK)
	    && ((sb_ptr->s_flag & JFS_BAD_SAIT) != JFS_BAD_SAIT)) {
		/*
		 * user specified 'only if dirty'
		 * and didn't specify 'omit logredo()'
		 * and logredo was successful
		 * and the aggregate is clean
		 */
		agg_recptr->fsck_is_done = 1;
		exit_value = FSCK_OK;
	} else if (agg_recptr->parm_options_logredo_only) {
		/*
		 * User only wants to run logredo, no matter what.
		 * Make sure we leave a dirty superblock marked dirty
		 */
		if (sb_ptr->s_state & FM_DIRTY)
			agg_recptr->ag_dirty = 1;
		agg_recptr->fsck_is_done = 1;
		exit_value = FSCK_OK;
	}
	/*
	 * if things look ok so far, make storage allocated by logredo()
	 * available to fsck processing.
	 */
	if (p0_rc == FSCK_OK) {
		p0_rc = release_logredo_allocs();
	}

	if (p0_rc != FSCK_OK) {
		agg_recptr->fsck_is_done = 1;
		exit_value = FSCK_OP_ERROR;
	}

	/*
	 * If we're done, make sure s_logdev is up to date in the superblock.
	 * Otherwise, s_logdev will be updated in final_processing.
	 */
	if (agg_recptr->fsck_is_done && (Log.location & OUTLINELOG)
	    && Log.devnum) {
		sb_ptr->s_logdev = Log.devnum;
		/*
		 * write the superblock to commit the above change
		 */
		if (use_2ndary_superblock) {
			/* put 2ndary */
			ujfs_put_superblk(Dev_IOPort, sb_ptr, 0);
		} else {
			/* put primary  */
			ujfs_put_superblk(Dev_IOPort, sb_ptr, 1);
		}
	}

	return (p0_rc);
}

/*****************************************************************************
 * NAME: phase1_processing
 *
 * FUNCTION:  Initialize the fsck workspace.  Process the aggregate-owned
 *            inodes.  Process fileset special inodes.
 *
 *            If any aggregate block is now multiply-allocated, then it
 *            is allocated to more than 1 special inode.  Exit.
 *
 *            Process all remaining inodes.  Count links from directories
 *            to their child inodes.
 *
 * PARAMETERS:  none
 *
 * RETURNS:
 *      success: FSCK_OK
 *      failure: something else
 */
int phase1_processing()
{
	int p1_rc = FSCK_OK;

	fsck_send_msg(fsck_PHASE1);

	if ((p1_rc = establish_io_buffers()) != FSCK_OK)
		goto p1_exit;

	/* I/O buffers established */
	/* establish workspace related to the aggregate */
	if ((p1_rc = establish_agg_workspace()) != FSCK_OK)
		goto p1_exit;

	/* aggregate workspace established */
	if ((p1_rc = record_fixed_metadata()) != FSCK_OK)
		goto p1_exit;

	/* fixed metadata recorded */
	/*
	 * note that this processing will use the vlarge
	 * buffer and then return it for reuse
	 */
	if ((p1_rc = validate_select_agg_inode_table()) != FSCK_OK)
		goto p1_exit;

	/* we have an ait to work with */
	/* establish workspace related to the fileset */
	if ((p1_rc = establish_fs_workspace()) != FSCK_OK)
		goto p1_exit;

	if ((p1_rc = record_dupchk_inode_extents()) != FSCK_OK)
		goto p1_exit;

	/* fs workspace established */
	/* claim the vlarge buffer for
	 * validating EAs.  We do this now because
	 * it IS possible that the root directory (validated
	 * in the call that follows) has an EA attached.
	 */
	establish_ea_iobuf();

	if ((p1_rc = allocate_dir_index_buffers()) != FSCK_OK)
		goto p1_exit;

	/* verify the metadata
	 * inodes for all filesets in the aggregate
	 */
	if ((p1_rc = validate_fs_metadata()) != FSCK_OK)
		goto p1_exit;

	/* check for blocks allocated
	 * to 2 or more metadata objects
	 */
	if ((p1_rc = fatal_dup_check()) != FSCK_OK)
		goto p1_exit;

	/* validate the fileset inodes */
	p1_rc = validate_fs_inodes();

	/* return the vlarge buffer for reuse */
	agg_recptr->ea_buf_ptr = NULL;
	agg_recptr->ea_buf_length = 0;
	agg_recptr->vlarge_current_use = NOT_CURRENTLY_USED;

	p1_exit:
	if (p1_rc != FSCK_OK)
	{
		agg_recptr->fsck_is_done = 1;
		exit_value = FSCK_OP_ERROR;
	}
	return (p1_rc);
}

/*****************************************************************************
 * NAME: report_readait_error
 *
 * FUNCTION:  Report failure to read the Aggregate Inode Table
 *
 * PARAMETERS:  none
 *
 * RETURNS:
 *      success: FSCK_OK
 *      failure: something else
 */
void report_readait_error(int local_rc, int global_rc, int which_it)
{
	/*
	 * message to user
	 */
	fsck_send_msg(fsck_URCVREAD, fsck_ref_msg(fsck_metadata), Vol_Label);
	/*
	 * message to debugger
	 */
	fsck_send_msg(fsck_ERRONAITRD, global_rc, local_rc, which_it);
	return;
}

/*****************************************************************************
 * NAME: validate_fs_inodes
 *
 * FUNCTION:  Verify the fileset inodes and structures rooted in them.
 *
 * PARAMETERS:  none
 *
 * RETURNS:
 *      success: FSCK_OK
 *      failure: something else
 */
int validate_fs_inodes()
{
	int                         vfi_rc = FSCK_OK;
	uint32_t                    ino_idx;
	struct dinode              *ino_ptr;
	int                         which_it = FILESYSTEM_I;	/* in release 1 there is only 1 fileset */
	struct fsck_ino_msg_info    ino_msg_info;
	struct fsck_ino_msg_info   *msg_info_ptr;
	bool                        DoCheck;
	int                         i;

	msg_info_ptr = &ino_msg_info;
	/* all fileset owned */
	msg_info_ptr->msg_inopfx = fsck_fset_inode;

	/*
	 * get the first non-metadata inode after
	 * the fileset root directory
	 */
	vfi_rc = inode_get_first_fs(which_it, &ino_idx, &ino_ptr);
	while ((vfi_rc == FSCK_OK) && (ino_ptr != NULL))
	{
	  if(InodeListSize == 0)
	  {
	    DoCheck = true;
	  }
	  else
	  {
	    DoCheck = false;
	    for(i = 0; i < InodeListSize; i++)
	      if(ino_idx == InodeList[i])
	      {
	        DoCheck = true;
	        break;
	      }
	  }

	  if(DoCheck)
	  {
	    /* no fatal errors and haven't seen the last inode */
	    if(inode_is_in_use(ino_ptr, (uint32_t) ino_idx))
	    {
	      /* inode is in use */
	      vfi_rc = validate_record_fileset_inode((uint32_t) ino_idx, ino_idx, ino_ptr, msg_info_ptr);
	    }
	    else
	    {
	      /* inode is allocated but is not in use */
	      if (!agg_recptr->avail_inode_found)
	      {
	        /*
	         * this is the first allocated, available
	         * inode we've seen all day
	         */
	        agg_recptr->avail_inonum = (uint32_t) ino_idx;
	        agg_recptr->avail_inode_found = 1;
	      }
	    }
	  }

		if (vfi_rc == FSCK_OK)
		{
			vfi_rc = inode_get_next(&ino_idx, &ino_ptr);
		}
	}
	return (vfi_rc);
}

/*****************************************************************************
 * NAME: verify_parms
 *
 * FUNCTION:  Verify that mutually exclusive invocation parameters were not
 *            specified.  Determine the level of messaging to be used for
 *            this fsck session.
 *
 * PARAMETERS:  none
 *
 * RETURNS:
 *      success: FSCK_OK
 *      failure: something else
 */
int verify_parms()
{
	/*
	 * If -f was chosen, have it override -a, -p, -r by
	 * turning off UFS_CHKDSK_IFDIRTY to force a check
	 * regardless of the outcome after the log is replayed
	 */
	if (agg_recptr->parm_options[UFS_CHKDSK_LEVEL3]) {
		agg_recptr->parm_options[UFS_CHKDSK_LEVEL2] = -1;
		agg_recptr->parm_options[UFS_CHKDSK_LEVEL3] = 0;
		agg_recptr->parm_options[UFS_CHKDSK_IFDIRTY] = 0;
	}

	/*
	 * If the -n flag was specified, turn off -p
	 */
	if (agg_recptr->parm_options[UFS_CHKDSK_LEVEL0]) {
		/*
		 * If -n is specified by itself, don't replay the journal.
		 * If -n is specified with -a, -p, or -f, replay the journal
		 * but don't make any other changes
		 */
		if (agg_recptr->parm_options[UFS_CHKDSK_LEVEL2] == 0)
			agg_recptr->parm_options[UFS_CHKDSK_SKIPLOGREDO] = -1;
		agg_recptr->parm_options[UFS_CHKDSK_LEVEL2] = -1;
	}
	else if (agg_recptr->parm_options[UFS_CHKDSK_LEVEL2]) {
		agg_recptr->parm_options[UFS_CHKDSK_LEVEL1] = 0;
		agg_recptr->parm_options[UFS_CHKDSK_LEVEL2] = 0;
		agg_recptr->parm_options[UFS_CHKDSK_LEVEL3] = -1;
		/*
		 * we'll be doing the Bad Block List function as part
		 * of the -f processing.  Turn off the flag that specifies
		 * it.  This flag is only used when we're in read-only mode.
		 */
		agg_recptr->parm_options[UFS_CHKDSK_CLRBDBLKLST] = 0;
	} else {
		/*
		 * Set default value if none was specified.
		 * set the default to same as -p
		 */
		agg_recptr->parm_options[UFS_CHKDSK_LEVEL2] = -1;
		agg_recptr->parm_options[UFS_CHKDSK_IFDIRTY] = -1;
		fsck_send_msg(fsck_SESSPRMDFLT);
	}
	/*
	 * the parms are valid.  record the messaging level they imply.
	 */
	if ((agg_recptr->parm_options[UFS_CHKDSK_VERBOSE])
	    || (agg_recptr->parm_options[UFS_CHKDSK_DEBUG])) {
		agg_recptr->effective_msg_level = fsck_debug;
		msg_lvl = fsck_debug;
	}
	else {
		agg_recptr->effective_msg_level = fsck_quiet;
		msg_lvl = fsck_quiet;
	}

	return (FSCK_OK);
}

void fsck_usage()
{
  TAP_PrintNet("Usage:  %s [-afnpvV] [-j journal_device] [--omit_journal_replay] [--replay_journal_only] device", program_name);
  TAP_PrintNet("Emergency help:\n"
	       " -a                 Automatic repair.\n"
	       " -f                 Force check even if file system is marked clean.\n"
	       " -j journal_device  Specify external journal device.\n"
         " -i inode           Check specific inode, up to 10 -i options may be specified.\n"
	       " -n                 Check read only, make no changes to the file system.\n"
	       " -p                 Automatic repair.\n"
	       " -v                 Be verbose.\n"
	       " -V                 Print version information only.\n"
	       " --omit_journal_replay    Omit transaction log replay.\n"
	       " --replay_journal_only    Only replay the transaction log.\n");
	//exit(FSCK_USAGE_ERROR);
	return;
}
