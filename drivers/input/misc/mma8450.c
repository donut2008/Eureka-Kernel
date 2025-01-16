efine TC300K_MODE_GLOVE		(1 << 4)

/* connecter check */
#define SUB_DET_DISABLE			0
#define SUB_DET_ENABLE_CON_OFF	1
#define SUB_DET_ENABLE_CON_ON	2

/* firmware */
#define TC300K_FW_PATH_SDCARD	"/sdcard/tc300k.bin"

#define TK_UPDATE_PASS		0
#define TK_UPDATE_DOWN		1
#define TK_UPDATE_FAIL		2

/* ISP command */
#define TC300K_CSYNC1			0xA3
#define TC300K_CSYNC2			0xAC
#define TC300K_CSYNC3			0xA5
#define TC300K_CCFG				0x92
#define TC300K_PRDATA			0x81
#define TC300K_PEDATA			0x82
#define TC300K_PWDATA			0x83
#define TC300K_PECHIP			0x8A
#define TC300K_PEDISC			0xB0
#define TC300K_LDDATA			0xB2
#define TC300K_LDMODE			0xB8
#define TC300K_RDDATA			0xB9
#define TC300K_PCRST			0xB4
#define TC300K_PCRED			0xB5
#define TC300K_PCINC			0xB6
#define TC300K_RDPCH			0xBD

/* ISP delay */
#define TC300K_TSYNC1			300	/* us */
#define TC300K_TSYNC2			50	/* 1ms~50ms */
#define TC300K_TSYNC3			100	/* us */
#define TC300K_TDLY1			1	/* us */
#define TC300K_TDLY2			2	/* us */
#define TC300K_TFERASE			10	/* ms */
#define TC300K_TPROG			20	/* us */

#define TC300K_CHECKSUM_DELAY	500

enum {
	FW_INKERNEL,
	FW_SDCARD,
};

struct fw_image {
	u8 hdr_ver;
	u8 hdr_len;
	u16 first_fw_ver;
	u16 second_fw_ver;
	u16 third_ver;
	u32 fw_len;
	u16 checksum;
	u16 alignment_dummy;
	u8 data[0];
} __attribute__ ((packed));

#define TSK_RELEASE			0x00
#define TSK_PRESS			0x01
#define GRIP_RELEASE			0x00
#define GRIP_PRESS			0x01

struct tsk_event_val {
	u16 tsk_bitmap;
	u8 tsk_status;
	int tsk_keycode;
	char* tsk_keyname;
};

#ifdef FEATURE_GRIP_FOR_SAR
struct grip_event_val {
	u16 grip_bitmap;
	u8 grip_status;
	int grip_code;
	char* grip_name;
};
#endif

struct tsk_event_val tsk_ev_old[8] =
{
	{0x01, TSK_PRESS, KEY_BACK, "back"},
	{0x02, TSK_PRESS, KEY_RECENT, "recent"},
	{0x03, TSK_PRESS, KEY_DUMMY_BACK, "dummy_back"},
	{0x04, TSK_PRESS, KEY_DUMMY_MENU, "dummy_menu"},
	{0x09, TSK_RELEASE, KEY_BACK, "back"},
	{0x0A, TSK_RELEASE, KEY_RECENT, "recent"},
	{0x0B, TSK_RELEASE, KEY_DUMMY_BACK, "dummy_back"},
	{0x0C, TSK_RELEASE, KEY_DUMMY_MENU, "dummy_menu"}
};

#ifdef FEATURE_GRIP_FOR_SAR
struct grip_event_val grip_ev[4] =
{
	{0x01 << 0, GRIP_PRESS, KEY_CP_GRIP, "grip1"},
	{0x01 << 1, GRIP_PRESS, KEY_CP_GRIP, "grip2"},
	{0x01 << 4, GRIP_RELEASE, KEY_CP_GRIP, "grip1"},
	{0x01 << 5, GRIP_RELEASE, KEY_CP_GRIP, "grip2"},
};
#endif

struct tsk_event_val tsk_ev[4] =
{
	{0x01 << 0, TSK_PRESS, KEY_RECENT, "recent"},
	{0x01 << 1, TSK_PRESS, KEY_BACK, "back"},
	{0x01 << 4, TSK_RELEASE, KEY_RECENT, "recent"},
	{0x01 << 5, TSK_RELEASE, KEY_BACK, "back"}
};


struct tsk_event_val tsk_ev_swap[4] =
{
	{0x01 << 0, TSK_PRESS, KEY_BACK, "back"},
	{0x01 << 1, TSK_PRESS, KEY_RECENT, "recent"},
	{0x01 << 4, TSK_RELEASE, KEY_BACK, "back"},
	{0x01 << 5, TSK_RELEASE, KEY_RECENT, "recent"}
};

struct tc300k_data {
	struct device *sec_touchkey;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct tc300k_platform_data *pdata;
	struct mutex lock;
	struct mutex lock_fac;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct fw_image *fw_img;
	const struct firmware *fw;
	char phys[32];
	int irq;
	u16 checksum;
	u16 threhold;
	int mode;
	int (*power) (bool on);
	u8 fw_ver;
	u8 fw_ver_bin;
	u8 md_ver;
	u8 md_ver_bin;
	u8 fw_update_status;
	bool enabled;
	bool fw_downloding;
	bool glove_mode;
	bool led_on;
	bool flip_mode;

	int key_num;
	struct tsk_event_val *tsk_ev_val;

	struct pinctrl *pinctrl_i2c;
	struct pinctrl *pinctrl_irq;
	struct pinctrl_state *pin_state[4];

#ifdef FEATURE_GRIP_FOR_SAR
	struct wake_lock touchkey_wake_lock;
	u16 grip_p_thd;
	u16 grip_r_thd;
	u16 grip_n_thd;
	u16 grip_s1;
	u16 grip_s2;
	u16 grip_baseline;
	u16 grip_raw1;
	u16 grip_raw2;
	u16 grip_event;
	bool sar_mode;
	bool sar_enable;
	bool sar_enable_off;
	struct delayed_work debug_work;
	//struct completion resume_done;
	//bool is_lpm_suspend;

	int grip_num;
	struct grip_event_val *grip_ev_val;
	int irq_count;
	int abnormal_mode;
	s32 diff;
	s32 max_diff;

#if defined (CONFIG_VBUS_NOTIFIER)
	struct notifier_block vbus_nb;
#endif
#endif
};

extern struct class *sec_class;

char *str_states[] = {"on_irq", "off_irq", "on_i2c", "off_i2c"};
enum {
	I_STATE_ON_IRQ = 0,
	I_STATE_OFF_IRQ,
	I_STATE_ON_I2C,
	I_STATE_OFF_I2C,
};

static bool tc300k_power_enabled;
static bool tc300k_keyled_enabled;
const char *regulator_ic;
const char *regulator_led;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tc300k_early_suspend(struct early_suspend *h);
static void tc300k_late_resume(struct early_suspend *h);
#endif

static void tc300k_input_close(struct input_dev *dev);
static int tc300k_input_open(struct input_dev *dev);
static int tc300_pinctrl_init(struct tc300k_data *data);
static void tc300_config_gpio_i2c(struct tc300k_data *data, int onoff);
static int tc300_pinctrl(struct tc300k_data *data, int status);
static int read_tc350k_register_data(struct tc300k_data *data, int read_key_num, int read_offset);
static int tc300k_mode_enable(struct i2c_client *client, u8 cmd);

static int tc300k_mode_check(struct i2c_client *client)
{
	int mode = i2c_smbus_read_byte_data(client, TC300K_MODE);
	if (mode < 0)
		input_err(true, &client->dev, "%s: failed to read mode (%d)\n",
			__func__, mode);

	return mode;
}

#ifdef FEATURE_GRIP_FOR_SAR

static void tc300k_set_debug_work(struct tc300k_data *data, u8 enable,
		unsigned int time_ms)
{
	if (enable == true) {
		schedule_delayed_work(&data->debug_work,
			msecs_to_jiffies(time_ms));
	} else {
		cancel_delayed_work_sync(&data->debug_work);
	}
}

static void tc300k_debug_work_func(struct work_struct *work)
{
	struct tc300k_data *data = container_of((struct