 */
#define V4L2_PIX_FMT_DV       v4l2_fourcc('d', 'v', 's', 'd') /* 1394          */
#define V4L2_PIX_FMT_MPEG     v4l2_fourcc('M', 'P', 'E', 'G') /* MPEG-1/2/4 Multiplexed */
#define V4L2_PIX_FMT_H264     v4l2_fourcc('H', '2', '6', '4') /* H264 with start codes */
#define V4L2_PIX_FMT_H264_NO_SC v4l2_fourcc('A', 'V', 'C', '1') /* H264 without start codes */
#define V4L2_PIX_FMT_H264_MVC v4l2_fourcc('M', '2', '6', '4') /* H264 MVC */
#define V4L2_PIX_FMT_H263     v4l2_fourcc('H', '2', '6', '3') /* H263          */
#define V4L2_PIX_FMT_MPEG1    v4l2_fourcc('M', 'P', 'G', '1') /* MPEG-1 ES     */
#define V4L2_PIX_FMT_MPEG2    v4l2_fourcc('M', 'P', 'G', '2') /* MPEG-2 ES     */
#define V4L2_PIX_FMT_MPEG4    v4l2_fourcc('M', 'P', 'G', '4') /* MPEG-4 part 2 ES */
#define V4L2_PIX_FMT_XVID     v4l2_fourcc('X', 'V', 'I', 'D') /* Xvid           */
#define V4L2_PIX_FMT_VC1_ANNEX_G v4l2_fourcc('V', 'C', '1', 'G') /* SMPTE 421M Annex G compliant stream */
#define V4L2_PIX_FMT_VC1_ANNEX_L v4l2_fourcc('V', 'C', '1', 'L') /* SMPTE 421M Annex L compliant stream */
#define V4L2_PIX_FMT_VP8      v4l2_fourcc('V', 'P', '8', '0') /* VP8 */

/*  Vendor-specific formats   */
#define V4L2_PIX_FMT_CPIA1    v4l2_fourcc('C', 'P', 'I', 'A') /* cpia1 YUV */
#define V4L2_PIX_FMT_WNVA     v4l2_fourcc('W', 'N', 'V', 'A') /* Winnov hw compress */
#define V4L2_PIX_FMT_SN9C10X  v4l2_fourcc('S', '9', '1', '0') /* SN9C10x compression */
#define V4L2_PIX_FMT_SN9C20X_I420 v4l2_fourcc('S', '9', '2', '0') /* SN9C20x YUV 4:2:0 */
#define V4L2_PIX_FMT_PWC1     v4l2_fourcc('P', 'W', 'C', '1') /* pwc older webcam */
#define V4L2_PIX_FMT_PWC2     v4l2_fourcc('P', 'W', 'C', '2') /* pwc newer webcam */
#define V4L2_PIX_FMT_ET61X251 v4l2_fourcc('E', '6', '2', '5') /* ET61X251 compression */
#define V4L2_PIX_FMT_SPCA501  v4l2_fourcc('S', '5', '0', '1') /* YUYV per line */
#define V4L2_PIX_FMT_SPCA505  v4l2_fourcc('S', '5', '0', '5') /* YYUV per line */
#define V4L2_PIX_FMT_SPCA508  v4l2_fourcc('S', '5', '0', '8') /* YUVY per line */
#define V4L2_PIX_FMT_SPCA561  v4l2_fourcc('S', '5', '6', '1') /* compressed GBRG bayer */
#define V4L2_PIX_FMT_PAC207   v4l2_fourcc('P', '2', '0', '7') /* compressed BGGR bayer */
#define V4L2_PIX_FMT_MR97310A v4l2_fourcc('M', '3', '1', '0') /* compressed BGGR bayer */
#define V4L2_PIX_FMT_JL2005BCD v4l2_fourcc('J', 'L', '2', '0') /* compressed RGGB bayer */
#define V4L2_PIX_FMT_SN9C2028 v4l2_fourcc('S', 'O', 'N', 'X') /* compressed GBRG bayer */
#define V4L2_PIX_FMT_SQ905C   v4l2_fourcc('9', '0', '5', 'C') /* compressed RGGB bayer */
#define V4L2_PIX_FMT_PJPG     v4l2_fourcc('P', 'J', 'P', 'G') /* Pixart 73xx JPEG */
#define V4L2_PIX_FMT_OV511    v4l2_fourcc('O', '5', '1', '1') /* ov511 JPEG */
#define V4L2_PIX_FMT_OV518    v4l2_fourcc('O', '5', '1', '8') /* ov518 JPEG */
#define V4L2_PIX_FMT_STV0680  v4l2_fourcc('S', '6', '8', '0') /* stv0680 bayer */
#define V4L2_PIX_FMT_TM6000   v4l2_fourcc('T', 'M', '6', '0') /* tm5600/tm60x0 */
#define V4L2_PIX_FMT_CIT_YYVYUY v4l2_fourcc('C', 'I', 'T', 'V') /* one line of Y then 1 line of VYUY */
#define V4L2_PIX_FMT_KONICA420  v4l2_fourcc('K', 'O', 'N', 'I') /* YUV420 planar in blocks of 256 pixels */
#define V4L2_PIX_FMT_JPGL	v4l2_fourcc('J', 'P', 'G', 'L') /* JPEG-Lite */
#define V4L2_PIX_FMT_SE401      v4l2_fourcc('S', '4', '0', '1') /* se401 janggu compressed rgb */
#define V4L2_PIX_FMT_S5C_UYVY_JPG v4l2_fourcc('S', '5', 'C', 'I') /* S5C73M3 interleaved UYVY/JPEG */
#define V4L2_PIX_FMT_Y8I      v4l2_fourcc('Y', '8', 'I', ' ') /* Greyscale 8-bit L/R interleaved */
#define V4L2_PIX_FMT_Y12I     v4l2_fourcc('Y', '1', '2', 'I') /* Greyscale 12-bit L/R interleaved */
#define V4L2_PIX_FMT_Z16      v4l2_fourcc('Z', '1', '6', ' ') /* Depth data 16-bit */

/* SDR formats - used only for Software Defined Radio devices */
#define V4L2_SDR_FMT_CU8          v4l2_fourcc('C', 'U', '0', '8') /* IQ u8 */
#define V4L2_SDR_FMT_CU16LE       v4l2_fourcc('C', 'U', '1', '6') /* IQ u16le */
#define V4L2_SDR_FMT_CS8          v4l2_fourcc('C', 'S', '0', '8') /* complex s8 */
#define V4L2_SDR_FMT_CS14LE       v4l2_fourcc('C', 'S', '1', '4') /* complex s14le */
#define V4L2_SDR_FMT_RU12LE       v4l2_fourcc('R', 'U', '1', '2') /* real u12le */

/* priv field value to indicates that subsequent fields are valid. */
#define V4L2_PIX_FMT_PRIV_MAGIC		0xfeedcafe

/* Flags */
#define V4L2_PIX_FMT_FLAG_PREMUL_ALPHA	0x00000001

/*
 *	F O R M A T   E N U M E R A T I O N
 */
struct v4l2_fmtdesc {
	__u32		    index;             /* Format number      */
	__u32		    type;              /* enum v4l2_buf_type */
	__u32               flags;
	__u8		    description[32];   /* Description string */
	__u32		    pixelformat;       /* Format fourcc      */
	__u32		    reserved[4];
};

#define V4L2_FMT_FLAG_COMPRESSED 0x0001
#define V4L2_FMT_FLAG_EMULATED   0x0002

#if 1
	/* Experimental Frame Size and frame rate enumeration */
/*
 *	F R A M E   S I Z E   E N U M E R A T I O N
 */
enum v4l2_frmsizetypes {
	V4L2_FRMSIZE_TYPE_DISCRETE	= 1,
	V4L2_FRMSIZE_TYPE_CONTINUOUS	= 2,
	V4L2_FRMSIZE_TYPE_STEPWISE	= 3,
};

struct v4l2_frmsize_discrete {
	__u32			width;		/* Frame width [pixel] */
	__u32			height;		/* Frame height [pixel] */
};

struct v4l2_frmsize_stepwise {
	__u32			min_width;	/* Minimum frame width [pixel] */
	__u32			max_width;	/* Maximum frame width [pixel] */
	__u32			step_width;	/* Frame width step size [pixel] */
	__u32			min_height;	/* Minimum frame height [pixel] */
	__u32			max_height;	/* Maximum frame height [pixel] */
	__u32			step_height;	/* Frame height step size [pixel] */
};

struct v4l2_frmsizeenum {
	__u32			index;		/* Frame size number */
	__u32			pixel_format;	/* Pixel format */
	__u32			type;		/* Frame size type the device supports. */

	union {					/* Frame size */
		struct v4l2_frmsize_discrete	discrete;
		struct v4l2_frmsize_stepwise	stepwise;
	};

	__u32   reserved[2];			/* Reserved space for future use */
};

/*
 *	F R A M E   R A T E   E N U M E R A T I O N
 */
enum v4l2_frmivaltypes {
	V4L2_FRMIVAL_TYPE_DISCRETE	= 1,
	V4L2_FRMIVAL_TYPE_CONTINUOUS	= 2,
	V4L2_FRMIVAL_TYPE_STEPWISE	= 3,
};

struct v4l2_frmival_stepwise {
	struct v4l2_fract	min;		/* Minimum frame interval [s] */
	struct v4l2_fract	max;		/* Maximum frame interval [s] */
	struct v4l2_fract	step;		/* Frame interval step size [s] */
};

struct v4l2_frmivalenum {
	__u32			index;		/* Frame format index */
	__u32			pixel_format;	/* Pixel format */
	__u32			width;		/* Frame width */
	__u32			height;		/* Frame height */
	__u32			type;		/* Frame interval type the device supports. */

	union {					/* Frame interval */
		struct v4l2_fract		discrete;
		struct v4l2_frmival_stepwise	stepwise;
	};

	__u32	reserved[2];			/* Reserved space for future use */
};
#endif

/*
 *	T I M E C O D E
 */
struct v4l2_timecode {
	__u32	type;
	__u32	flags;
	__u8	frames;
	__u8	seconds;
	__u8	minutes;
	__u8	hours;
	__u8	userbits[4];
};

/*  Type  */
#define V4L2_TC_TYPE_24FPS		1
#define V4L2_TC_TYPE_25FPS		2
#define V4L2_TC_TYPE_30FPS		3
#define V4L2_TC_TYPE_50FPS		4
#define V4L2_TC_TYPE_60FPS		5

/*  Flags  */
#define V4L2_TC_FLAG_DROPFRAME		0x0001 /* "drop-frame" mode */
#define V4L2_TC_FLAG_COLORFRAME		0x0002
#define V4L2_TC_USERBITS_field		0x000C
#define V4L2_TC_USERBITS_USERDEFINED	0x0000
#define V4L2_TC_USERBITS_8BITCHARS	0x0008
/* The above is based on SMPTE timecodes */

struct v4l2_jpegcompression {
	int quality;

	int  APPn;              /* Number of APP segment to be written,
				 * must be 0..15 */
	int  APP_len;           /* Length of data in JPEG APPn segment */
	char APP_data[60];      /* Data in the JPEG APPn segment. */

	int  COM_len;           /* Length of data in JPEG COM segment */
	char COM_data[60];      /* Data in JPEG COM segment */

	__u32 jpeg_markers;     /* Which markers should go into the JPEG
				 * output. Unless you exactly know what
				 * you do, leave them untouched.
				 * Including less markers will make the
				 * resulting code smaller, but there will
				 * be fewer applications which can read it.
				 * The presence of the APP and COM marker
				 * is influenced by APP_len and COM_len
				 * ONLY, not by this property! */

#define V4L2_JPEG_MARKER_DHT (1<<3)    /* Define Huffman Tables */
#define V4L2_JPEG_MARKER_DQT (1<<4)    /* Define Quantization Tables */
#define V4L2_JPEG_MARKER_DRI (1<<5)    /* Define Restart Interval */
#define V4L2_JPEG_MARKER_COM (1<<6)    /* Comment segment */
#define V4L2_JPEG_MARKER_APP (1<<7)    /* App segment, driver will
					* always use APP0 */
};

/*
 *	M E M O R Y - M A P P I N G   B U F F E R S
 */
struct v4l2_requestbuffers {
	__u32			count;
	__u32			type;		/* enum v4l2_buf_type */
	__u32			memory;		/* enum v4l2_memory */
	__u32			reserved[2];
};

/**
 * struct v4l2_plane - plane info for multi-planar buffers
 * @bytesused:		number of bytes occupied by data in the plane (payload)
 * @length:		size of this plane (NOT the payload) in bytes
 * @mem_offset:		when memory in the associated struct v4l2_buffer is
 *			V4L2_MEMORY_MMAP, equals the offset from the start of
 *			the device memory for this plane (or is a "cookie" that
 *			should be passed to mmap() called on the video node)
 * @userptr:		when memory is V4L2_MEMORY_USERPTR, a userspace pointer
 *			pointing to this plane
 * @fd:			when memory is V4L2_MEMORY_DMABUF, a userspace file
 *			descriptor associated with this plane
 * @data_offset:	offset in the plane to the start of data; usually 0,
 *			unless there is a header in front of the data
 *
 * Multi-planar buffers consist of one or more planes, e.g. an YCbCr buffer
 * with two planes can have one plane for Y, and another for interleaved CbCr
 * components. Each plane can reside in a separate memory buffer, or even in
 * a completely separate memory node (e.g. in embedded devices).
 */
struct v4l2_plane {
	__u32			bytesused;
	__u32			length;
	union {
		__u32		mem_offset;
		unsigned long	userptr;
		__s32		fd;
	} m;
	__u32			data_offset;
	__u32			reserved[11];
};

/**
 * struct v4l2_buffer - video buffer info
 * @index:	id number of the buffer
 * @type:	enum v4l2_buf_type; buffer type (type == *_MPLANE for
 *		multiplanar buffers);
 * @bytesused:	number of bytes occupied by data in the buffer (payload);
 *		unused (set to 0) for multiplanar buffers
 * @flags:	buffer informational flags
 * @field:	enum v4l2_field; field order of the image in the buffer
 * @timestamp:	frame timestamp
 * @timecode:	frame timecode
 * @sequence:	sequence count of this frame
 * @memory:	enum v4l2_memory; the method, in which the actual video data is
 *		passed
 * @offset:	for non-multiplanar buffers with memory == V4L2_MEMORY_MMAP;
 *		offset from the start of the device memory for this plane,
 *		(or a "cookie" that should be passed to mmap() as offset)
 * @userptr:	for non-multiplanar buffers with memory == V4L2_MEMORY_USERPTR;
 *		a userspace pointer pointing to this buffer
 * @fd:		for non-multiplanar buffers with memory == V4L2_MEMORY_DMABUF;
 *		a userspace file descriptor associated with this buffer
 * @planes:	for multiplanar buffers; userspace pointer to the array of plane
 *		info structs for this buffer
 * @length:	size in bytes of the buffer (NOT its payload) for single-plane
 *		buffers (when type != *_MPLANE); numbe