ULCALIBUR V Stick", 0, XTYPE_XBOX360 },
	{ 0x24c6, 0x5510, "Hori Fighting Commander ONE (Xbox 360/PC Mode)", MAP_TRIGGERS_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x24c6, 0x550d, "Hori GEM Xbox controller", 0, XTYPE_XBOX360 },
	{ 0x24c6, 0x550e, "Hori Real Arcade Pro V Kai 360", MAP_TRIGGERS_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x24c6, 0x551a, "PowerA FUSION Pro Controller", 0, XTYPE_XBOXONE },
	{ 0x24c6, 0x561a, "PowerA FUSION Controller", 0, XTYPE_XBOXONE },
	{ 0x24c6, 0x5b00, "ThrustMaster Ferrari 458 Racing Wheel", 0, XTYPE_XBOX360 },
	{ 0x24c6, 0x5b02, "Thrustmaster, Inc. GPX Controller", 0, XTYPE_XBOX360 },
	{ 0x24c6, 0x5b03, "Thrustmaster Ferrari 458 Racing Wheel", 0, XTYPE_XBOX360 },
	{ 0x24c6, 0x5d04, "Razer Sabertooth", 0, XTYPE_XBOX360 },
	{ 0x24c6, 0xfafe, "Rock Candy Gamepad for Xbox 360", 0, XTYPE_XBOX360 },
	{ 0x2563, 0x058d, "OneXPlayer Gamepad", 0, XTYPE_XBOX360 },
	{ 0x2dc8, 0x2000, "8BitDo Pro 2 Wired Controller fox Xbox", 0, XTYPE_XBOXONE },
	{ 0x31e3, 0x1100, "Wooting One", 0, XTYPE_XBOX360 },
	{ 0x31e3, 0x1200, "Wooting Two", 0, XTYPE_XBOX360 },
	{ 0x31e3, 0x1210, "Wooting Lekker", 0, XTYPE_XBOX360 },
	{ 0x31e3, 0x1220, "Wooting Two HE", 0, XTYPE_XBOX360 },
	{ 0x31e3, 0x1300, "Wooting 60HE (AVR)", 0, XTYPE_XBOX360 },
	{ 0x31e3, 0x1310, "Wooting 60HE (ARM)", 0, XTYPE_XBOX360 },
	{ 0x3285, 0x0607, "Nacon GC-100", 0, XTYPE_XBOX360 },
	{ 0x3767, 0x0101, "Fanatec Speedster 3 Forceshock Wheel", 0, XTYPE_XBOX },
	{ 0xffff, 0xffff, "Chinese-made Xbox Controller", 0, XTYPE_XBOX },
	{ 0x0000, 0x0000, "Generic X-Box pad", 0, XTYPE_UNKNOWN }
};

/* buttons shared with xbox and xbox360 */
static const signed short xpad_common_btn[] = {
	BTN_A, BTN_B, BTN_X, BTN_Y,			/* "analog" buttons */
	BTN_START, BTN_SELECT, BTN_THUMBL, BTN_THUMBR,	/* start/back/sticks */
	-1						/* terminating entry */
};

/* original xbox controllers only */
static const signed short xpad_btn[] = {
	BTN_C, BTN_Z,		/* "analog" buttons */
	-1			/* terminating entry */
};

/* used when dpad is mapped to buttons */
static const signed short xpad_btn_pad[] = {
	BTN_TRIGGER_HAPPY1, BTN_TRIGGER_HAPPY2,		/* d-pad left, right */
	BTN_TRIGGER_HAPPY3, BTN_TRIGGER_HAPPY4,		/* d-pad up, down */
	-1				/* terminating entry */
};

/* used when triggers are mapped to buttons */
static const signed short xpad_btn_triggers[] = {
	BTN_TL2, BTN_TR2,		/* triggers left/right */
	-1
};

static const signed short xpad360_btn[] = {  /* buttons for x360 controller */
	BTN_TL, BTN_TR,		/* Button LB/RB */
	BTN_MODE,		/* The big X button */
	-1
};

static const signed short xpad_abs[] = {
	ABS_X, ABS_Y,		/* left stick */
	ABS_RX, ABS_RY,		/* right stick */
	-1			/* terminating entry */
};

/* used when dpad is mapped to axes */
static const signed short xpad_abs_pad[] = {
	ABS_HAT0X, ABS_HAT0Y,	/* d-pad axes */
	-1			/* terminating entry */
};

/* used when triggers are mapped to axes */
static const signed short xpad_abs_triggers[] = {
	ABS_Z, ABS_RZ,		/* triggers left/right