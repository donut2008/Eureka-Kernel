V4L2_BAND_MODULATION_VSB	(1 << 1)
#define V4L2_BAND_MODULATION_FM		(1 << 2)
#define V4L2_BAND_MODULATION_AM		(1 << 3)

struct v4l2_frequency_band {
	__u32	tuner;
	__u32	type;	/* enum v4l2_tuner_type */
	__u32	index;
	__u32	capability;
	__u32	rangelow;
	__u32	rangehigh;
	__u32	modulation;
	__u32	reserved[9];
};

struct v4l2_hw_freq_seek {
	__u32	tuner;
	__u32	type;	/* enum v4l2_tuner_type */
	__u32	seek_upward;
	__u32	wrap_around;
	__u32	spacing;
	__u32	rangelow;
	__u32	rangehigh;
	__u32	reserved[5];
};

/*
 *	R D S
 */

struct v4l2_rds_data {
	__u8 	lsb;
	__u8 	msb;
	__u8 	block;
} __attribute__ ((packed));

#define V4L2_RDS_BLOCK_MSK 	 0x7
#define V4L2_RDS_BLOCK_A 	 0
#define V4L2_RDS_BLOCK_B 	 1
#define V4L2_R