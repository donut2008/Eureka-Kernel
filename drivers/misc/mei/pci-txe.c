PACE_LOCK_ENABLE	0x28

static const u16 NCT6791_REG_WEIGHT_TEMP_SEL[6] = { 0, 0x239 };
static const u16 NCT6791_REG_WEIGHT_TEMP_STEP[6] = { 0, 0x23a };
static const u16 NCT6791_REG_WEIGHT_TEMP_STEP_TOL[6] = { 0, 0x23b };
static const u16 NCT6791_REG_WEIGHT_DUTY_STEP[6] = { 0, 0x23c };
static const u16 NCT6791_REG_WEIGHT_TEMP_BASE[6] = { 0, 0x23d };
static const u16 NCT6791_REG_WEIGHT_DUTY_BASE[6] = { 0, 0x23e };

static const u16 NCT6791_REG_ALARM[NUM_REG_ALARM] = {
	0x459, 0x45A, 0x45B, 0x568, 0x45D };

static const s8 NCT6791_ALARM_BITS[] = {
	0, 1, 2, 3, 8, 21, 20, 16,	/* in0.. in7 */
	17, 24, 25, 26, 27, 28, 29,	/* in8..in14 */
	-1,				/* unused */
	6, 7, 11, 10, 23, 33,		/* fan1..fan6 */
	-1, -1,				/* unused */
	4, 5, 13, -1, -1, -1,		/* temp1..temp6 */
	12, 9 };			/* intrusion0, intrusion1 */

/* NCT6792/NCT6793 specific data */

static const u16 NCT6792_REG_TEMP_MON[] = {
	0x73, 0x75, 0x77, 0x79, 0x7b, 0x7d };
static const u16 NCT6792_REG_BEEP[NUM_REG_BEEP] = {
	0xb2, 0xb3, 0xb4, 0xb5, 0xbf };

static const char *const nct6792_temp_label[] = {
	"",
	"SYSTIN",
	"CPUTIN",
	"AUXTIN0",
	"AUXTIN1",
	"AUXTIN2",
	"AUXTIN3",
	"",
	"SMBUSMASTER 0",
	"SMBUSMASTER 1",
	"SMBUSMASTER 2",
	"SMBUSMASTER 3",
	"SMBUSMASTER 4",
	"SMBUSMASTER 5",
	"SMBUSMASTER 6",
	"SMBUSMASTER 7",
	"PECI Agent 0",
	"PECI Agent 1",
	"PCH_CHIP_CPU_MAX_TEMP",
	"PCH_CHIP_TEMP",
	"PCH_CPU_TEMP",
	"PCH_MCH_TEMP",
	"PCH_DIM0_TEMP",
	"PCH_DIM1_TEMP",
	"PCH_DIM2_TEMP",
	"PCH_DIM3_TEMP",
	"BYTE_TEMP",
	"PECI Agent 0 Calibration",
	"PECI Agent 1 Calibration",
	"",
	"",
	"Virtual_TEMP"
};

static const char *const nct6793_temp_label[] = {
	"",
	"SYSTIN",
	"CPUTIN",
	"AUXTIN0",
	"AUXTIN1",
	"AUXTIN2",
	"AUXTIN3",
	"",
	"SMBUSMASTER 0",
	"SMBUSMASTER 1",
	"",
	"",
	"",
	"",
	"",
	"",
	"PECI Agent 0",
	"PECI Agent 1",
	"PCH_CHIP_CPU_MAX_TEMP",
	"PCH_CHIP_TEMP",
	"PCH_CPU_TEMP",
	"PCH_MCH_TEMP",
	"Agent0 Dimm0 ",
	"Agent0 Dimm1",
	"Agent1 Dimm0",
	"Agent1 Dimm1",
	"BYTE_TEMP0",
	"BYTE_TEMP1",
	"PECI Agent 0 Calibration",
	"PECI Agent 1 Calibration",
	"",
	"Virtual_TEMP"
};

/* NCT6102D/NCT6106D specific data */

#define NCT6106_REG_VBAT	0x318
#define NCT6106_REG_DIODE	0x319
#define NCT6106_DIODE_MASK	0x01

static const u16 NCT6106_REG_IN_MAX[] = {
	0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9e, 0xa0, 0xa2 };
static const u16 NCT6106_REG_IN_MIN[] = {
	0x91, 0x93, 0x95, 0x97, 0x99, 0x9b, 0x9f, 0xa1, 0xa3 };
static const u16 NCT6106_REG_IN[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x09 };

static const u16 NCT6106_REG_TEMP[] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 };
static const u16 NCT6106_REG_TEMP_MON[] = { 0x18, 0x19, 0x1a };
static const u16 NCT6106_REG_TEMP_HYST[] = {
	0xc3, 0xc7, 0xcb, 0xcf, 0xd3, 0xd7 };
static const u16 NCT6106_REG_TEMP_OVER[] = {
	0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd6 };
static const u16 NCT6106_REG_TEMP_CRIT_L[] = {
	0xc0, 0xc4, 0xc8, 0xcc, 0xd0, 0xd4 };
static const u16 NCT6106_REG_TEMP_CRIT_H[] = {
	0xc1, 0xc5, 0xc9, 0xcf, 0xd1, 0xd5 };
static const u16 NCT6106_REG_TEMP_OFFSET[] = { 0x311, 0x312, 0x313 };
static const u16 NCT6106_REG_TEMP_CONFIG[] = {
	0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc };

static const u16 NCT6106_REG_FAN[] = { 0x20, 0x22, 0x24 };
static const u16 NCT6106_REG_FAN_MIN[] = { 0xe0, 0xe2, 0xe4 };
static const u16 NCT6106_REG_FAN_PULSES[] = { 0xf6, 0xf6, 0xf6, 0, 0 };
static const u16 NCT6106_FAN_PULSE_SHIFT[] = { 0, 2, 4, 0, 0 };

static const u8 NCT6106_REG_PWM_MODE[] = { 0xf3, 0xf3, 0xf3 };
static const u8 NCT6106_PWM_MODE_MASK[] = { 0x01, 0x02, 0x04 };
static const u16 NCT6106_REG_PWM[] = { 0x119, 0x129, 0x139 };
static const u16 NCT6106_REG_PWM_READ[] = { 0x4a, 0x4b, 0x4c };
static const u16 NCT6106_REG_FAN_MODE[] = { 0x113, 0x123, 0x133 };
static const u16 NCT6106_REG_TEMP_SEL[] = { 0x110, 0x120, 0x130 };
static const u16 NCT6106_REG_TEMP_SOURCE[] = {
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5 };

static const u16 NCT6106_REG_CRITICAL_TEMP[] = { 0x11a, 0x12a, 0x13a };
static const u16 NCT6106_REG_CRITICAL_TEMP_TOLERANCE[] = {
	0x11b, 0x12b, 0x13b };

static const u16 NCT6106_REG_CRITICAL_PWM_ENABLE[] = { 0x11c, 0x12c, 0x13c };
#define NCT6106_CRITICAL_PWM_ENABLE_MASK	0x10
static const u16 NCT6106_REG_CRITICAL_PWM[] = { 0x11d, 0x12d, 0x13d };

static const u16 NCT6106_REG_FAN_STEP_UP_TIME[] = { 0x114, 0x124, 0x134 };
static const u16 NCT6106_REG_FAN_STEP_DOWN_TIME[] = { 0x115, 0x125, 0x135 };
static const u16 NCT6106_REG_FAN_STOP_OUTPUT[] = { 0x116, 0x126, 0x136 };
static const u16 NCT6106_REG_FAN_START_OUTPUT[] = { 0x117, 0x127, 0x137 };
static const u16 NCT6106_REG_FAN_STOP_TIME[] = { 0x118, 0x128, 0x138 };
static const u16 NCT6106_REG_TOLERANCE_H[] = { 0x112, 0x122, 0x132 };

static const u16 NCT6106_REG_TARGET[] = { 0x111, 0x121, 0x131 };

static const u16 NCT6106_REG_WEIGHT_TEMP_SEL[] = { 0x168, 0x178, 0x188 };
static const u16 NCT6106_REG_WEIGHT_TEMP_STEP[] = { 0x169, 0x179, 0x189 };
static const u16 NCT6106_REG_WEIGHT_TEMP_STEP_TOL[] = { 0x16a, 0x17a, 0x18a };
static const u16 NCT6106_REG_WEIGHT_DUTY_STEP[] = { 0x16b, 0x17b, 0x18b };
static const u16 NCT6106_REG_WEIGHT_TEMP_BASE[] = { 0x16c, 0x17c, 0x18c };
static const u16 NCT6106_REG_WEIGHT_DUTY_BASE[] = { 0x16d, 0x17d, 0x18d };

static const u16 NCT6106_REG_AUTO_TEMP[] = { 0x160, 0x170, 0x180 };
static const u16 NCT6106_REG_AUTO_PWM[] = { 0x164, 0x174, 0x184 };

static const u16 NCT6106_REG_ALARM[NUM_REG_ALARM] = {
	0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d };

static const s8 NCT6106_ALARM_BITS[] = {
	0, 1, 2, 3, 4, 5, 7, 8,		/* in0.. in7 */
	9, -1, -1, -1, -1, -1, -1,	/* in8..in14 */
	-1,				/* unused */
	32, 33, 34, -1, -1,		/* fan1..fan5 */
	-1, -1, -1,			/* unused */
	16, 17, 18, 19, 20, 21,		/* temp1..temp6 */
	48, -1				/* intrusion0, intrusion1 */
};

static const u16 NCT6106_REG_BEEP[NUM_REG_BEEP] = {
	0x3c0, 0x3c1, 0x3c2, 0x3c3, 0x3c4 };

static const s8 NCT6106_BEEP_BITS[] = {
	0, 1, 2, 3, 4, 5, 7, 8,		/* in0.. in7 */
	9, 10, 11, 12, -1, -1, -1,	/* in8..in14 */
	32,				/* global beep enable */
	24, 25, 26, 27, 28,		/* fan1..fan5 */
	-1, -1, -1,			/* unused */
	16, 17, 18, 19, 20, 21,		/* temp1..temp6 */
	34, -1				/* intrusion0, intrusion1 */
};

static const u16 NCT6106_REG_TEMP_ALTERNATE[ARRAY_SIZE(nct6776_temp_label) - 1]
	= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x51, 0x52, 0x54 };

static const u16 NCT6106_REG_TEMP_CRIT[ARRAY_SIZE(nct6776_temp_label) - 1]
	= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x204, 0x205 };

static enum pwm_enable reg_to_pwm_enable(int pwm, int mode)
{
	if (mode == 0 && pwm == 255)
		return off;
	return mode + 1;
}

static int pwm_enable_to_reg(enum pwm_enable mode)
{
	if (mode == off)
		return 0;
	return mode - 1;
}

/*
 * Conversions
 */

/* 1 is DC mode, output in ms */
static unsigned int step_time_from_reg(u8 reg, u8 mode)
{
	return mode ? 400 * reg : 100 * reg;
}

static u8 step_time_to_reg(unsigned int msec, u8 mode)
{
	return clamp_val((mode ? (msec + 200) / 400 :
					(msec + 50) / 100), 1, 255);
}

static unsigned int fan_from_reg8(u16 reg, unsigned int divreg)
{
	if (reg == 0 || reg == 255)
		return 0;
	return 1350000U / (reg << divreg);
}

static unsigned int fan_from_reg13(u16 reg, unsigned int divreg)
{
	if ((reg & 0xff1f) == 0xff1f)
		return 0;

	reg = (reg & 0x1f) | ((reg & 0xff00) >> 3);

	if (reg == 0)
		return 0;

	return 1350000U / reg;
}

static unsigned int fan_from_reg16(u16 reg, unsigned int divreg)
{
	if (reg == 0 || reg == 0xffff)
		return 0;

	/*
	 * Even though the registers are 16 bit wide, the fan divisor
	 * still applies.
	 */
	return 1350000U / (reg << divreg);
}

static u16 fan_to_reg(u32 fan, unsigned int divreg)
{
	if (!fan)
		return 0;

	return (1350000U / fan) >> divreg;
}

static inline unsigned int
div_from_reg(u8 reg)
{
	return 1 << reg;
}

/*
 * Some of the voltage inputs have internal scaling, the tables below
 * contain 8 (the ADC LSB in mV) * scaling factor * 100
 */
static const u16 scale_in[15] = {
	800, 800, 1600, 1600, 800, 800, 800, 1600, 1600, 800, 800, 800, 800,
	800, 800
};

static inline long in_from_reg(u8 reg, u8 nr)
{
	return DIV_ROUND_CLOSEST(reg * scale_in[nr], 100);
}

static inline u8 in_to_reg(u32 val, u8 nr)
{
	return clamp_val(DIV_ROUND_CLOSEST(val * 100, scale_in[nr]), 0, 255);
}

/*
 * Data structures and manipulation thereof
 */

struct nct6775_data {
	int addr;	/* IO base of hw monitor block */
	int sioreg;	/* SIO register address */
	enum kinds kind;
	const char *name;

	const struct attribute_group *groups[6];

	u16 reg_temp[5][NUM_TEMP]; /* 0=temp, 1=temp_over, 2=temp_hyst,
				    * 3=temp_crit, 4=temp_lcrit
				    */
	u8 temp_src[NUM_TEMP];
	u16 reg_temp_config[NUM_TEMP];
	const char * const *temp_label;
	int temp_label_num;

	u16 REG_CONFIG;
	u16 REG_VBAT;
	u16 REG_DIODE;
	u8 DIODE_MASK;

	const s8 *ALARM_BITS;
	const s8 *BEEP_BITS;

	const u16 *REG_VIN;
	const u16 *REG_IN_MINMAX[2];

	const u16 *REG_TARGET;
	const u16 *REG_FAN;
	const u16 *REG_FAN_MODE;
	const u16 *REG_FAN_MIN;
	const u16 *REG_FAN_PULSES;
	const u16 *FAN_PULSE_SHIFT;
	const u16 *REG_FAN_TIME[3];

	const u16 *REG_TOLERANCE_H;

	const u8 *REG_PWM_MODE;
	const u8 *PWM_MODE_MASK;

	const u16 *REG_PWM[7];	/* [0]=pwm, [1]=pwm_start, [2]=pwm_floor,
				 * [3]=pwm_max, [4]=pwm_step,
				 * [5]=weight_duty_step, [6]=weight_duty_base
				 */
	const u16 *REG_PWM_READ;

	const u16 *REG_CRITICAL_PWM_ENABLE;
	u8 CRITICAL_PWM_ENABLE_MASK;
	const u16 *REG_CRITICAL_PWM;

	const u16 *REG_AUTO_TEMP;
	const u16 *REG_AUTO_PWM;

	const u16 *REG_CRITICAL_TEMP;
	const u16 *REG_CRITICAL_TEMP_TOLERANCE;

	const u16 *REG_TEMP_SOURCE;	/* temp register sources */
	const u16 *REG_TEMP_SEL;
	const u16 *REG_WEIGHT_TEMP_SEL;
	const u16 *REG_WEIGHT_TEMP[3];	/* 0=base, 1=toler