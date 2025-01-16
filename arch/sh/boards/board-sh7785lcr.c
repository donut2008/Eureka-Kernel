contained within is corrupted. */
#define V4L2_BUF_FLAG_ERROR			0x00000040
/* timecode field is valid */
#define V4L2_BUF_FLAG_TIMECODE			0x00000100
/* Buffer is prepared for queuing */
#define V4L2_BUF_FLAG_PREPARED			0x00000400
/* Cache handling flags */
#define V4L2_BUF_FLAG_NO_CACHE_INVALIDATE	0x00000800
#define V4L2_BUF_FLAG_NO_CACHE_CLEAN		0x00001000
/* Timestamp type */
#define V4L2_BUF_FLAG_TIMESTAMP_MASK		0x0000e000
#define V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN		0x00000000
#define V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC	0x00002000
#define V4L2_BUF_FLAG_TIMESTAMP_COPY		0x00004000
/* Expects and returns an Android sync fence */
#define V4L2_BUF_FLAG_USE_SYNC         0x00008000
/* Timestamp sources. */
#define V4L2_BUF_FLAG_TSTAMP_SRC_MASK		0x00070000
#define V4L2_BUF_FLAG_TSTAMP_SRC_EOF		0x00000000
#define V4L2_BUF_FLAG_TSTAMP_SRC_SOE		0x00010000
/* mem2mem encoder/decoder */
#define V4L2_BUF_FLAG_LAST			0x00100000

/**
 * struct v4l2_exportbuffer - export of video buffer as DMABUF file descriptor
 *
 * @index:	id number of the buffer
 * @type:	enum v4l2_buf_type; buffer type (type == *_MPLANE for
 *		multiplanar buffers);
 * @plane:	index of the plane to be exported, 0 for single plane queues
 * @flags:	flags for newly created file, currently only O_CLOEXEC is
 *		supported, refer to manual of open syscall for more details
 * @fd:		file descriptor associated with DMABUF (set by driver)
 *
 * Contains data used for exporting a video buffer as DMABUF file descriptor.
 * The buffer is identified by a 'cookie' returned by VIDIOC_QUERYBUF
 * (identical to the cookie used to mmap() the buffer to userspace). All
 * reserved fields must be set to zero. The field reserved0 is expected to
 * become a structure 'type' allowing an alternative layout of the structure
 * content. Therefore this field should not be used for any other extensions.
 */
struct v4l2_exportbuffer {
	__u32		type; /* enum v4l2_buf_type */
	__u32		index;
	__u32		plane;
	__u32		flags;
	__s32		fd;
	__u32		reserved[11];
};

/*
 *	O V E R L A Y   P R E V I E W
 */
struct v4l2_framebuffer {
	__u32			capability;
	__u32			flags;
/* FIXME: in theory we should pass something like PCI device + memory
 * region + offset instead of some physical address */
	void                    *base;
	struct {
		__u32		width;
		__u32		height;
		__u32		pixelformat;
		__u32		field;		/* enum v4l2_field */
		__u32		bytesperline;	/* for padding, zero if unused */
		__u32		sizeimage;
		__u32		colorspace;	/* enum v4l2_colorspace */
		__u32		priv;		/* reserved field, set to 0 */
	} fmt;
};
/*  Flags for the 'capability' field. Read only */
#define V4L2_FBUF_CAP_EXTERNOVERLAY	0x0001
#define V4L2_FBUF_CAP_CHROMAKEY		0x0002
#define V4L2_FBUF_CAP_LIST_CLIPPING     0x0004
#define V4L2_FBUF_CAP_BITMAP_CLIPPING	0x0008
#define V4L2_FBUF_CAP_LOCAL_ALPHA	0x0010
#define V4L2_FBUF_CAP_GLOBAL_ALPHA	0x0020
#define V4L2_FBUF_CAP_LOCAL_INV_ALPHA	0x0040
#define V4L2_FBUF_CAP_SRC_CHROMAKEY	0x0080
/*  Flags for the 'flags' field. */
#define V4L2_FBUF_FLAG_PRIMARY		0x0001
#define V4L2_FBUF_FLAG_OVERLAY		0x0002
#define V4L2_FBUF_FLAG_CHROMAKEY	0x0004
#define V4L2_FBUF_FLAG_LOCAL_ALPHA	0x0008
#define V4L2_FBUF_FLAG_GLOBAL_ALPHA	0x0010
#define V4L2_FBUF_FLAG_LOCAL_INV_ALPHA	0x0020
#define V4L2_FBUF_FLAG_SRC_CHROMAKEY	0x0040

struct v4l2_clip {
	struct v4l2_rect        c;
	struct v4l2_clip	__user *next;
};

struct v4l2_window {
	struct v4l2_rect        w;
	__u32			field;	 /* enum v4l2_field */
	__u32			chromakey;
	struct v4l2_clip	__user *clips;
	__u32			clipcount;
	void			__user *bitmap;
	__u8                    global_alpha;
};

/*
 *	C A P T U R E   P A R A M E T E R S
 */
struct v4l2_captureparm {
	__u32		   capability;	  /*  Supported modes */
	__u32		   capturemode;	  /*  Current mode */
	struct v4l2_fract  timeperframe;  /*  Time per frame in seconds */
	__u32		   extendedmode;  /*  Driver-specific extensions */
	__u32              readbuffers;   /*  # of buffers for read */
	__u32		   reserved[4];
};

/*  Flags for 'capability' and 'capturemode' fields */
#define V4L2_MODE_HIGHQUALITY	0x0001	/*  High quality imaging mode */
#define V4L2_CAP_TIMEPERFRAME	0x1000	/*  timeperframe field is supported */

struct v4l2_outputparm {
	__u32		   capability;	 /*  Supported modes */
	__u32		   outputmode;	 /*  Current mode */
	struct v4l2_fract  timeperframe; /*  Time per frame in seconds */
	__u32		   extendedmode; /*  Driver-specific extensions */
	__u32              writebuffers; /*  # of buffers for write */
	__u32		   reserved[4];
};

/*
 *	I N P U T   I M A G E   C R O P P I N G
 */
struct v4l2_cropcap {
	__u32			type;	/* enum v4l2_buf_type */
	struct v4l2_rect        bounds;
	struct v4l2_rect        defrect;
	struct v4l2_fract       pixelaspect;
};

struct v4l2_crop {
	__u32			type;	/* enum v4l2_buf_type */
	struct v4l2_rect        c;
};

/**
 * struct v4l2_selection - selection info
 * @type:	buffer type (do not use *_MPLANE types)
 * @target:	Selection target, used to choose one of possible rectangles;
 *		defined in v4l2-common.h; V4L2_SEL_TGT_* .
 * @flags:	constraints flags, defined in v4l2-common.h; V4L2_SEL_FLAG_*.
 * @r:		coordinates of selection window
 * @reserved:	for future use, rounds structure size to 64 bytes, set to zero
 *
 * Hardware may use multiple helper windows to process a video stream.
 * The structure is used to exchange this selection areas between
 * an application and a driver.
 */
struct v4l2_selection {
	__u32			type;
	__u32			target;
	__u32                   flags;
	struct v4l2_rect        r;
	__u32                   reserved[9];
};


/*
 *      A N A L O G   V I D E O   S T A N D A R D
 */

typedef __u64 v4l2_std_id;

/* one bit for each */
#define V4L2_STD_PAL_B          ((v4l2_std_id)0x00000001)
#define V4L2_STD_PAL_B1         ((v4l2_std_id)0x00000002)
#define V4L2_STD_PAL_G          ((v4l2_std_id)0x00000004)
#define V4L2_STD_PAL_H          ((v4l2_std_id)0x00000008)
#define V4L2_STD_PAL_I          ((v4l2_std_id)0x00000010)
#define V4L2_STD_PAL_D          ((v4l2_std_id)0x00000020)
#define V4L2_STD_PAL_D1         ((v4l2_std_id)0x00000040)
#define V4L2_STD_PAL_K          ((v4l2_std_id)0x00000080)

#define V4L2_STD_PAL_M          ((v4l2_std_id)0x00000100)
#define V4L2_STD_PAL_N          ((v4l2_std_id)0x00000200)
#define V4L2_STD_PAL_Nc         ((v4l2_std_id)0x00000400)
#define V4L2_STD_PAL_60         ((v4l2_std_id)0x00000800)

#define V4L2_STD_NTSC_M         ((v4l2_std_id)0x00001000)	/* BTSC */
#define V4L2_STD_NTSC_M_JP      ((v4l2_std_id)0x00002000)	/* EIA-J */
#define V4L2_STD_NTSC_443       ((v4l2_std_id)0x00004000)
#define V4L2_STD_NTSC_M_KR      ((v4l2_std_id)0x00008000)	/* FM A2 */

#define V4L2_STD_SECAM_B        ((v4l2_std_id)0x00010000)
#define V4L2_STD_SECAM_D        ((v4l2_std_id)0x00020000)
#define V4L2_STD_SECAM_G        ((v4l2_std_id)0x00040000)
#define V4L2_STD_SECAM_H        ((v4l2_std_id)0x00080000)
#define V4L2_STD_SECAM_K        ((v4l2_std_id)0x00100000)
#define V4L2_STD_SECAM_K1       ((v4l2_std_id)0x00200000)
#define V4L2_STD_SECAM_L        ((v4l2_std_id)0x00400000)
#define V4L2_STD_SECAM_LC       ((v4l2_std_id)0x00800000)

/* ATSC/HDTV */
#define V4L2_STD_ATSC_8_VSB     ((v4l2_std_id)0x01000000)
#define V4L2_STD_ATSC_16_VSB    ((v4l2_std_id)0x02000000)

/* FIXME:
   Although std_id is 64 bits, there is an issue on PPC32 architecture that
   makes switch(__u64) to break. So, there's a hack on v4l2-common.c rounding
   this value to 32 bits.
   As, currently, the max value is for V4L2_STD_ATSC_16_VSB (30 bits wide),
   it should work fine. However, if needed to add more than two standards,
   v4l2-common.c should be fixed.
 */

/*
 * Some macros to merge video standards in order to make live easier for the
 * drivers and V4L2 applications
 */

/*
 * "Common" NTSC/M - It should be noticed that V4L2_STD_NTSC_443 is
 * Missing here.
 */
#define V4L2_STD_NTSC           (V4L2_STD_NTSC_M	|\
				 V4L2_STD_NTSC_M_JP     |\
				 V4L2_STD_NTSC_M_KR)
/* Secam macros */
#define V4L2_STD_SECAM_DK      	(V4L2_STD_SECAM_D	|\
				 V4L2_STD_SECAM_K	|\
				 V4L2_STD_SECAM_K1)
/* All Secam Standards */
#define V4L2_STD_SECAM		(V4L2_STD_SECAM_B	|\
				 V4L2_STD_SECAM_G	|\
				 V4L2_STD_SECAM_H	|\
				 V4L2_STD_SECAM_DK	|\
				 V4L2_STD_SECAM_L       |\
				 V4L2_STD_SECAM_LC)
/* PAL macros */
#define V4L2_STD_PAL_BG		(V4L2_STD_PAL_B		|\
				 V4L2_STD_PAL_B1	|\
				 V4L2_STD_PAL_G)
#define V4L2_STD_PAL_DK		(V4L2_STD_PAL_D		|\
				 V4L2_STD_PAL_D1	|\
				 V4L2_STD_PAL_K)
/*
 * "Common" PAL - This macro is there to be compatible with the old
 * V4L1 concept of "PAL": /BGDKHI.
 * Several PAL standards are missing here: