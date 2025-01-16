eg);
	return inb(ioreg + 1);
}

static inline void
superio_select(int ioreg, int ld)
{
	outb(SIO_REG_LDSEL, ioreg);
	outb(ld, ioreg + 1);
}

static inline int
superio_enter(int ioreg)
{
	/*
	 * Try to reserve <ioreg> and <ioreg + 1> for exclusive access.
	 */
	if (!request_muxed_region(ioreg, 2, DRVNAME))
		return -EBUSY;

	outb(0x87, ioreg);
	outb(0x87, ioreg);

	return 0;
}

static inline void
superio_exit(int ioreg)
{
	outb(0xaa, ioreg);
	outb(0x02, ioreg);
	outb(0x02, ioreg + 1);
	release_region(ioreg, 2);
}

/*
 * ISA constants
 */

#define IOREGION_ALIGNMENT	(~7)
#define IOREGION_OFFSET		5
#define IOREGION_LENGTH		2
#define ADDR_REG_OFFSET		0
#define DATA_REG_OFFSET		1

#define NCT6775_REG_BANK	0x4E
#define NCT6775_REG_CONFIG	0x40

/*
 * Not currently used:
 * REG_MAN_ID has the value 0x5ca3 for all supported chips.
 * REG_CHIP_ID == 0x88/0xa1/0xc1 depending on chip model.
 * REG_MAN_ID is at port 0x4f
 * REG_CHIP_ID is at port 0x58
 */

#define NUM_TEMP	10	/* Max number of temp attribute sets w/ limits*/
#define NUM_TEMP_FIXED	6	/* Max number of fixed temp attribute sets */

#define NUM_REG_ALARM	7	/* Max number of alarm registers */
#define NUM_REG_BEEP	5	/* Max number of beep registers */

#define NUM_FAN		6

/* Common and NCT6775 specific data */

/* Voltage min/max registers for nr=7..14 are in bank 5 */

static const u16 NCT6775_REG_IN_MAX[] = {
	0x2b, 0x2d, 0x2f, 0x31, 0x33, 0x35, 0x37, 0x554, 0x556, 0x558, 0x55a,
	0x55c, 0x55e, 0x560, 0x562 };
static const u16 NCT6775_REG_IN_MIN[] = {
	0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x555, 0x557, 0x559, 0x55b,
	0x55d, 0x55f, 0x561, 0x563 };
static const u16 NCT6775_REG_IN[] = {
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x550, 0x551, 0x552
};

#define NCT6775_REG_VBAT		0x5D
#define NCT6775_REG_DIODE		0x5E
#define NCT6775_DIODE_MASK		0x02

#define NCT6775_REG_FANDIV1		0x506
#define NCT6775_REG_FANDIV2		0x507

#define NCT6775_REG_CR_FAN_DEBOUNCE	0xf0

static const u16 NCT6775_REG_ALARM[NUM_REG_ALARM] = { 0x459, 0x45A, 0x45B };

/* 0..15 voltages, 16..23 fans, 24..29 temperatures, 30..31 intrusion */

static const s8 NCT6775_ALARM_BITS[] = {
	0, 1, 2, 3, 8, 21, 20, 16,	/* in0.. in7 */
	17, -1, -1, -1, -1, -1, -1,	/* in8..in14 */
	-1,				/* unused */
	6, 7, 11, -1, -1,		/* fan1..fan5 */
	-1, -1, -1,			/* unused */
	4, 5, 13, -1, -1, -1,		/* temp1..temp6 */
	12, -1 };			/* intrusion0, intrusion1 */

#define FAN_ALARM_BASE		16
#define TEMP_ALARM_BASE		24
#define INTRUSION_ALARM_BASE	30

static const u16 NCT6775_REG_BEEP[NUM_REG_BEEP] = { 0x56, 0x57, 0x453, 0x4e };

/*
 * 0..14 voltages, 15 global beep enable, 16..23 fans, 24..29 temperatures,
 * 30..31 intrusion
 */
static const s8 NCT6775_BEEP_BITS[] = {
	0, 1, 2, 3, 8, 9, 10, 16,	/* in0.. in7 */
	17, -1, -1, -1, -1, -1, -1,	/* in8..in14 */
	21,				/* global beep enable */
	6, 7, 11, 28, -1,		/* fan1..fan5 */
	-1, -1, -1,			/* unused */
	4, 5, 13, -1, -1, -1,		/* temp1..temp6 */
	12, -1 };			/* intrusion0, intrusion1 */

#define BEEP_ENABLE_BASE		15

static const u8 NCT6775_REG_CR_CASEOPEN_CLR[] = { 0xe6, 0xee };
static const u8 NCT6775_CR_CASEOPEN_CLR_MASK[] = { 0x20, 0x01 };

/* DC or PWM output fan configuration */
static const u8 NCT6775_REG_PWM_MODE[] = { 0x04, 0x04, 0x12 };
static const u8 NCT6775_PWM_MODE_MASK[] = { 0x01, 0x02, 0x01 };

/* Advanced Fan control, some values are common for all fans */

static const u16 NCT6775_REG_TARGET[] = {
	0x101, 0x201, 0x301, 0x801, 0x901, 0xa01 };
static const u16 NCT6775_REG_FAN_MODE[] = {
	0x102, 0x202, 0x302, 0x802, 0x902, 0xa02 };
static const u16 NCT6775_REG_FAN_STEP_DOWN_TIME[] = {
	0x103, 0x203, 0x303, 0x803, 0x903, 0xa03 };
static const u16 NCT6775_REG_FAN_STEP_UP_TIME[] = {
	0x104, 0x204, 0x304, 0x804, 0x904, 0xa04 };
static const u16 NCT6775_REG_FAN_STOP_OUTPUT[] = {
	0x105, 0x205, 0x305, 0x805, 0x905, 0xa05 };
static const u16 NCT6775_REG_FAN_START_OUTPUT[] = {
	0x106, 0x206, 0x306, 0x806, 0x906, 0xa06 };
static const u16 NCT6775_REG_FAN_MAX_OUTPUT[] = { 0x10a, 0x20a, 0x30a };
static const u16 NCT6775_REG_FAN_STEP_OUTPUT[] = { 0x10b, 0x20b, 0x30b };

static const u16 NCT6775_REG_FAN_STOP_TIME[] = {
	0x107, 0x207, 0x307, 0x807, 0x907, 0xa07 };
static const u16 NCT6775_REG_PWM[] = {
	0x109, 0x209, 0x309, 0x809, 0x909, 0xa09 };
static const u16 NCT6775_REG_PWM_READ[] = {
	0x01, 0x03, 0x11, 0x13, 0x15, 0xa09 };

static const u16 NCT6775_REG_FAN[] = { 0x630, 0x632, 0x634, 0x636, 0x638 };
static const u16 NCT6775_REG_FAN_MIN[] = { 0x3b, 0x3c, 0x3d };
static const u16 NCT6775_REG_FAN_PULSES[] = { 0x641, 0x642, 0x643, 0x644, 0 };
static const u16 NCT6775_FAN_PULSE_SHIFT[] = { 0, 0, 0, 0, 0, 0 };

static const u16 NCT6775_REG_TEMP[] = {
	0x27, 0x150, 0x250, 0x62b, 0x62c, 0x62d };

static const u16 NCT6775_REG_TEMP_MON[] = { 0x73, 0x75, 0x77 };

static const u16 NCT6775_REG_TEMP_CONFIG[ARRAY_SIZE(NCT6775_REG_TEMP)] = {
	0, 0x152, 0x252, 0x628, 0x629, 0x62A };
static const u16 NCT6775_REG_TEMP_HYST[ARRAY_SIZE(NCT6775_REG_TEMP)] = {
	0x3a, 0x153, 0x253, 0x673, 0x678, 0x67D };
static const u16 NCT6775_REG_TEMP_OVER[ARRAY_SIZE(NCT6775_REG_TEMP)] = {
	0x39, 0x155, 0x255, 0x672, 0x677, 0x67C };

static const u16 NCT6775_REG_TEMP_SOURCE[ARRAY_SIZE(NCT6775_REG_TEMP)] = {
	0x621, 0x622, 0x623, 0x624, 0x625, 0x626 };

static const u16 NCT6775_REG_TEMP_SEL[] = {
	0x100, 0x200, 0x300, 0x800, 0x900, 0xa00 };

static const u16 NCT6775_REG_WEIGHT_TEMP_SEL[] = {
	0x139, 0x239, 0x339, 0x839, 0x939, 0xa39 };
static const u16 NCT6775_REG_WEIGHT_TEMP_STEP[] = {
	0x13a, 0x23a, 0x33a, 0x83a, 0x93a, 0xa3a };
static const u16 NCT6775_REG_WEIGHT_TEMP_STEP_TOL[] = {
	0x13b, 0x23b, 0x33b, 0x83b, 0x93b, 0xa3b };
static const u16 NCT6775_REG_WEIGHT_DUTY_STEP[] = {
	0x13c, 0x23c, 0x33c, 0x83c, 0x93c, 0xa3c };
static const u16 NCT6775_REG_WEIGHT_TEMP_BASE[] = {
	0x13d, 0x23d, 0x33d, 0x83d, 0x93d, 0xa3d };

static const u16 NCT6775_REG_TEMP_OFFSET[] = { 0x454, 0x455, 0x456 };

static const u16 NCT6775_REG_AUTO_TEMP[] = {
	0x121, 0x221, 0x321, 0x821, 0x921, 0xa21 };
static const u16 NCT6775_REG_AUTO_PWM[] = {
	0x127, 0x227, 0x327, 0x827, 0x927, 0xa27 };

#define NCT6775_AUTO_TEMP(data, nr, p)	((data)->REG_AUTO_TEMP[nr] + (p))
#define NCT6775_AUTO_PWM(data, nr, p)	((data)->REG_AUTO_PWM[nr] + (p))

static const u16 NCT6775_REG_CRITICAL_ENAB[] = { 0x134, 0x234, 0x334 };

static const u16 NCT6775_REG_CRITICAL_TEMP[] = {
	0x135, 0x235, 0x335, 0x835, 0x935, 0xa35 };
static const u16 NCT6775_REG_CRITICAL_TEMP_TOLERANCE[] = {
	0x138, 0x238, 0x338, 0x838, 0x938, 0xa38 };

static const char *const nct6775_temp_label[] = {
	"",
	"SYSTIN",
	"CPUTIN",
	"AUXTIN",
	"AMD SB-TSI",
	"PECI Agent 0",
	"PECI Agent 1",
	"PECI Agent 2",
	"PECI Agent 3",
	"PECI Agent 4",
	"PECI Agent 5",
	"PECI Agent 6",
	"PECI Agent 7",
	"PCH_CHIP_CPU_MAX_TEMP",
	"PCH_CHIP_TEMP",
	"PCH_CPU_TEMP",
	"PCH_MCH_TEMP",
	"PCH_DIM0_TEMP",
	"PCH_DIM1_TEMP",
	"PCH_DIM2_TEMP",
	"PCH_DIM3_TEMP"
};

static const u16 NCT6775_REG_TEMP_ALTERNATE[ARRAY_SIZE(nct6775_temp_label) - 1]
	= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x661, 0x662, 0x664 };

static const u16 NCT6775_REG_TEMP_CRIT[ARRAY_SIZE(nct6775_temp_label) - 1]
	= { 0, 0, 0, 0, 0xa00, 0xa01, 0xa02, 0xa03, 0xa04, 0xa05, 0xa06,
	    0xa07 };

/* NCT6776 specific data */

/* STEP_UP_TIME and STEP_DOWN_TIME regs are swapped for all chips but NCT6775 */
#define NCT6776_REG_FAN_STEP_UP_TIME NCT6775_REG_FAN_STEP_DOWN_TIME
#define NCT6776_REG_FAN_STEP_DOWN_TIME NCT6775_REG_FAN_STEP_UP_TIME

static const s8 NCT6776_ALARM_BITS[] = {
	0, 1, 2, 3, 8, 21, 20, 16,	/* in0.. in7 */
	17, -1, -1, -1, -1, -1, -1,	/* in8..in14 */
	-1,				/* unused */
	6, 7, 11, 10, 23,		/* fan1..fan5 */
	-1, -1, -1,			/* unused */
	4, 5, 13, -1, -1, -1,		/* temp1..temp6 */
	12, 9 };			/* intrusion0, intrusion1 */

static const u16 NCT6776_REG_BEEP[NUM_REG_BEEP] = { 0xb2, 0xb3, 0xb4, 0xb5 };

static const s8 NCT6776_BEEP_BITS[] = {
	0, 1, 2, 3, 4, 5, 6, 7,		/* in0.. in7 */
	8, -1, -1, -1, -1, -1, -1,	/* in8..in14 */
	24,				/* global beep enable */
	25, 26, 27, 28, 29,		/* fan1..fan5 */
	-1, -1, -1,			/* unused */
	16, 17, 18, 19, 20, 21,		/* temp1..temp6 */
	30, 31 };			/* intrusion0, intrusion1 */

static const u16 NCT6776_REG_TOLERANCE_H[] = {
	0x10c, 0x20c, 0x30c, 0x80c, 0x90c, 0xa0c };

static const u8 NCT6776_REG_PWM_MODE[] = { 0x04, 0, 0, 0, 0, 0 };
static const u8 NCT6776_PWM_MODE_MASK[] = { 0x01, 0, 0, 0, 0, 0 };

static const u16 NCT6776_REG_FAN_MIN[] = { 0x63a, 0x63c, 0x63e, 0x640, 0x642 };
static const u16 NCT6776_REG_FAN_PULSES[] = { 0x644, 0x645, 0x646, 0, 0 };

static const u16 NCT6776_REG_WEIGHT_DUTY_BASE[] = {
	0x13e, 0x23e, 0x33e, 0x83e, 0x93e, 0xa3e };

static const u16 NCT6776_REG_TEMP_CONFIG[ARRAY_SIZE(NCT6775_REG_TEMP)] = {
	0x18, 0x152, 0x252, 0x628, 0