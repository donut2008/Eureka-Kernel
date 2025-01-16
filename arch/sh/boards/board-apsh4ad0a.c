ive polarity */
#define V4L2_DV_VSYNC_POS_POL	0x00000001
#define V4L2_DV_HSYNC_POS_POL	0x00000002

/* Timings standards */
#define V4L2_DV_BT_STD_CEA861	(1 << 0)  /* CEA-861 Digital TV Profile */
#define V4L2_DV_BT_STD_DMT	(1 << 1)  /* VESA Discrete Monitor Timings */
#define V4L2_DV_BT_STD_CVT	(1 << 2)  /* VESA Coordinated Video Timings */
#define V4L2_DV_BT_STD_GTF	(1 << 3)  /* VESA Generalized Timings Formula */

/* Flags */

/* CVT/GTF specific: timing uses reduced blanking (CVT) or the 'Secondary
   GTF' curve (GTF). In both cases the horizontal and/or vertical blanking
   intervals are reduced, allowing a higher resolution over the same
   bandwidth. This is a read-only flag. */
#define V4L2_DV_FL_REDUCED_BLANKING		(1 << 0)
/* CEA-861 specific: set for CEA-861 formats with a framerate of a multiple
   of six. These formats can be optionally played at 1 / 1.001 speed.
   This is a read-only flag. */
#define V4L2_DV_FL_CAN_REDUCE_FPS		(1 << 1)
/* CEA-861 specific: only valid for video transmitters, the flag is cleared
   by receivers.
   If the framerate of the format is a multiple of six, then the pixelclock
   used to set up the transmitter is divided by 1.001 to make it compatible
   with 60 Hz based standards such as NTSC and PAL-M that use a framerate of
   29.97 Hz. Otherwise this flag is cleared. If the transmitter can't generate
   such frequencies, then the flag will also be cleared. */
#define V4L2_DV_FL_REDUCED_FPS			(1 << 2)
/* Specific to interlaced formats: if set, then field 1 is really one half-line
   longer and field 2 is really one half-line shorter, so each field has
   exactly the same number of half-lines. Whether half-lines can be detected
   or used depends on the hardware. */
#define V4L2_DV_FL_HALF_LINE			(1 << 3)
/* If set, then this is a Consumer Electronics (CE) video format. Such formats
 * differ from other formats (commonly called IT formats) in that if RGB
 * encoding is used then by default the RGB values use limited range (i.e.
 * use the range 16-235) as opposed to 0-255. All formats defined in CEA-861
 * except for the 640x480 format are CE formats. */
#define V4L2_DV_FL_IS_CE_VIDEO			(1 << 4)

/* A few useful defines to calculate the total blanking and frame sizes */
#define V4L2_DV_BT_BLANKING_WIDTH(bt) \
	((bt)->hfrontporch + (bt)->hsync + (bt)->hbackporch)
#define V4L2_DV_BT_FRAME_WIDTH(bt) \
	((bt)->width + V4L2_DV_BT_BLANKING_WIDTH(bt))
#define V4L2_DV_BT_BLANKING_HEIGHT(bt) \
	((bt)->vfrontporch + (bt)->vsync + (bt)->vbackporch + \
	 ((bt)->interlaced ? \
	  ((bt)->il_vfrontporch + (bt)->il_vsync + (bt)->il_vbackporch) : 0))
#define V4L2_DV_BT_FRAME_HEIGHT(bt) \
	((bt)->height + V4L2_DV_BT_BLANKING_HEIGHT(bt))

/** struct v4l2_dv_timings - DV timings
 * @type:	the type of the timings
 * @bt:	BT656/1120 timings
 */
struct v4l2_dv_timings {
	__u32 type;
	union {
		struct v4l2_bt_timings	bt;
		__u32	reserved[32];
	};
} __attribute__ ((packed));

/* Values for the type field */
#define V4L2_DV_BT_656_1120	0	/* BT.656/1120 timing type */


/** struct v4l2_enum_dv_timings - DV timings enumeration
 * @index:	enumeration index
 * @pad:	the pad number for which to enumerate timings (used with
 *		v4l-subdev nodes only)
 * @reserved:	must be zeroed
 * @timings:	the timings for the given index
 */
struct v4l2_enum_dv_timings {
	__u32 index;
	__u32 pad;
	__u32 reserved[2];
	struct v4l2_dv_timings timings;
};