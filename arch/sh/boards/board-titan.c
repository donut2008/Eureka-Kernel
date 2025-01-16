aced formats */
#define V4L2_DV_BT_CAP_INTERLACED	(1 << 0)
/* Supports progressive formats */
#define V4L2_DV_BT_CAP_PROGRESSIVE	(1 << 1)
/* Supports CVT/GTF reduced blanking */
#define V4L2_DV_BT_CAP_REDUCED_BLANKING	(1 << 2)
/* Supports custom formats */
#define V4L2_DV_BT_CAP_CUSTOM		(1 << 3)

/** struct v4l2_dv_timings_cap - DV timings capabilities
 * @type:	the type of the timings (same as in struct v4l2_dv_timings)
 * @pad:	the pad number for which to query capabilities (used with
 *		v4l-subdev nodes only)
 * @bt:		the BT656/1120 timings capabilities
 */
struct v4l2_dv_timings_cap {
	__u32 type;
	