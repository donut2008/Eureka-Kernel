f) >= 0x1000)
#define V4L2_CTRL_MAX_DIMS	  (4)

enum v4l2_ctrl_type {
	V4L2_CTRL_TYPE_INTEGER	     = 1,
	V4L2_CTRL_TYPE_BOOLEAN	     = 2,
	V4L2_CTRL_TYPE_MENU	     = 3,
	V4L2_CTRL_TYPE_BUTTON	     = 4,
	V4L2_CTRL_TYPE_INTEGER64     = 5,
	V4L2_CTRL_TYPE_CTRL_CLASS    = 6,
	V4L2_CTRL_TYPE_STRING        = 7,
	V4L2_CTRL_TYPE_BITMASK       = 8,
	V4L2_CTRL_TYPE_INTEGER_MENU  = 9,

	/* Compound types are >= 0x0100 */
	V4L2_CTRL_COMPOUND_TYPES     = 0x0100,
	V4L2_CTRL_TYPE_U8	     = 0x0100,
	V4L2_CTRL_TYPE_U16	     = 0x0101,
	V4L2_CTRL_TYPE_U32	     = 0x0102,
};

/*  Used in the VIDIOC_QUERYCTRL ioctl for querying controls */
struct v4l2_queryctrl {
	__u32		     id;
	__u32		     type;	/* enum v4l2_ctrl_type */
	__u8		     name[32];	/* Whatever */
	__s32		     minimum;	/* Note signedness */
	__s32		     maximum;
	__s32		     step;
	__s32		     default_value;
	__u32                flags;
	__u32		     reserved[2];
};

/*  Used in the VIDIOC_QUERY_EXT_CTRL ioctl for querying extended controls */
struct v4l2_query_ext_ctrl {
	__u32		     id;
	__u32		     type;
	char		     name[32];
	__s64		     minimum;
	__s64		     maximum;
	__u64		     step;
	__s64		     default_value;
	__u32                flags;
	__u32                elem_size;
	__u32                elems;
	__u32                nr_of_dims;
	__u32                dims[V4L2_CTRL_MAX_DIMS];
	__u32		     reserved[32];
};

/*  Used in the VIDIOC_QUERYMENU ioctl for querying menu items */
struct v4l2_querymenu {
	__u32		id;
	__u32		index;
	union {
		__u8	name[32];	/* Whatever */
		__s64	value;
	};
	__u32		reserved;
} __attribute__ ((packed));

/*  Control flags  */
#define V4L2_CTRL_FLAG_DISABLED		0x0001
#