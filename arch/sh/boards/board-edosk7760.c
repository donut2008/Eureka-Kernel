This implements is a proposal V4L2 API to allow SLICED VBI
 * required for some hardware encoders. It should change without
 * notice in the definitive implementation.
 */

struct v4l2_sliced_vbi_format {
	__u16   service_set;
	/* service_lines[0][...] specifies lines 0-23 (1-23 used) of the first field
	   service_lines[1][...] specifies lines 0-23 (1-23 used) of the second field
				 (equals frame lines 313-336 for 625 line video
				  standards, 263-286 for 525 line standards) */
	__u16   service_lines[2][24];
	__u32   io_size;
	__u32   reserved[2];            /* must be zero */
};

/* Teletext World System Teletext
   (WST), defined on ITU-R BT.653-2 */
#define V4L2_SLICED_TELETEXT_B          (0x0001)
/* Video Program System, defined on ETS 300 231*/
#define V4L2_SLICED_VPS                 (0x0400)
/* Closed Caption, defined on EIA-608 */
#define V4L2_SLICED_CAPTION_525         (0x1000)
/* Wide Screen System, defined on ITU-R BT1119.1 */
#define V4L2_SLICED_WSS_625             (0x4000)

#define V4L2_SLICED_VBI_525             (V4L2_SLICED_CAPTION_525)
#define V4L2_SLICED_VBI_625             (V4L2_SLICED_TELETEXT_B | V4L2_SLICED_VPS | V4L2_SLICED_WSS_625)

struct v4l2_sliced_vbi_cap {
	__u16   service_set;
	/* service_lines[0][...] specifies lines 0-23 (1-23 used) of the first field
	   service_lines[1][...] specifies lines 0-23 (1-23 used) of the second field
				 (equals frame lines 313-336 for 625 line video
				  standards, 263-286 for 525 line standards) */
	__u16   service_lines[2][24];
	__u32	type;		/* enum v4l2_buf_type */
	__u32   reserved[3];    /* must be 0 */
};

struct v4l2_sliced_vbi_data {
	__u32   id;
	__u32   field;          /* 0: first field, 1: second field */
	__u32   line;           /* 1-23 */
	__u32   reserved;       /* must be 0 */
	__u8    data[48];
};

/*
 * Sliced VBI data inserted into MPEG Streams
 */

/*
 * V4L2_MPEG_STREAM_VBI_FMT_IVTV:
 *
 * Structure of payload contained in an MPEG 2 Private Stream 1 PES Packet in an
 * MPEG-2 Program Pack that contains V4L2_MPEG_STREAM_VBI_FMT_IVTV Sliced VBI
 * data
 *
 * Note, the MPEG-2 Program Pack and Private Stream 1 PES packet header
 * definitions are not included here.  See the MPEG-2 specifications for details
 * on these headers.
 */

/* Line type IDs */
#define V4L2_MPEG_VBI_IVTV_TELETEXT_B     (1)
#define V4L2_MPEG_VBI_IVTV_CAPTION_525    (4)
#define V4L2_MPEG_VBI_IVTV_WSS_625        (5)
#define V4L2_MPEG_VBI_IVTV_VPS            (7)

struct v4l2_mpeg_vbi_itv0_line {
	__u8 id;	/* One of V4L2_MPEG_VBI_IVTV_* above */
	__u8 data[42];	/* Sliced VBI data for the line */
} __attribute__ ((packed));

struct v4l2_mpeg_vbi_itv0 {
	__le32 linemask[2]; /* Bitmasks of VBI service lines present */
	struct v4l2_mpeg_vbi_itv0_line line[35];
} __attribute__ ((packed));

struct v4l2_mpeg_vbi_ITV0 {
	struct v4l2_mpeg_vbi_itv0_line line[36];
} __attribute__ ((packed));

#define V4L2_MPEG_VBI_IVTV_MAGIC0	"itv0"
#define V4L2_MPEG_VBI_IVTV_MAGIC1	"ITV0"

struct v4l2_mpeg_vbi_fmt_ivtv {
	__u8 magic[4];
	union {
		struct v4l2_mpeg_vbi_itv0 itv0;
		struct v4l2_mpeg_vbi_ITV0 ITV0;
	};
} __attribute__ ((packed));

/*
 *	A G G R E G A T E   S T R U C T U R E S
 */

/**
 * struct v4l2_plane_pix_format - additional, per-plane format definition
 * @sizeimage:		maximum size in bytes required for data, for which
 *			this plane will be used
 * @bytesperline:	distance in bytes between the leftmost pixels in two
 *			adjacent lines
 */
struct v4l2_plane_pix_format {
	__u32		sizeimage;
	__u32		bytesperline;
	__u16		reserved[6];
} __attribute__ ((packed));

/**
 * struct v4l2_pix_format_mplane - multiplanar format definition
 * @width:		image width in pixels
 * @height:		image height in pixels
 * @pixelformat:	little endian four character code (fourcc)
 * @field:		enum v4l2_field; field order (for interlaced video)
 * @colorspace:		enum v4l2_colorspace; supplemental to pixelformat
 * @plane_fmt:		per-plane information
 * @num_planes:		number of planes for this format
 * @flags:		format flags (V4L2_PIX_FMT_FLAG_*)
 * @ycbcr_enc:		enum v4l2_ycbcr_encoding, Y'CbCr encoding
 * @quantization:	enum v4l2_quantization, colorspace quantization
 * @xfer_func:		enum v4l2_xfer_func, colorspace transfer function
 */
struct v4l2_pix_format_mplane {
	__u32				width;
	__u32				height;
	__u32				pixelformat;
	__u32				field;
	__u32				colorspace;

	struct v4l2_plane_pix_format	plane_fmt[VIDEO_MAX_PLANES];
	__u8				num_planes;
	__u8				flags;
	__u8				ycbcr_enc;
	__u8				quantization;
	__u8				xfer_func;
	__u8				reserved[7];
} __attribute__ ((packed));

/**
 * struct v4l2_sdr_format - SDR 