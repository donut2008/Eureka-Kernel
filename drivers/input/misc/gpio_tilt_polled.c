D >= 0) { // ***************** Checking Touch Event's ID whethere it is same with previous ID.
						if(!input_events[i].value && input_events[iTouchID].value < 0) {  // If this event is 'Release'
							for(j=0;j<MAX_MULTI_TOUCH_EVENTS;j++) {
								TouchIDs[j] = -1;
							}
						}
					}
					break;
				case BTN_TOOL_PEN :
					if(input_events[i].value && !hover_booster.multi_events) {
						pr_debug("[Input Booster] PEN EVENT - HOVER ON\n");
						RUN_BOOSTER(hover, BOOSTER_ON);
						DetectedCategory = true;
					} else if(!input_events[i].value && hover_booster.multi_events) {
						pr_debug("[Input Booster] PEN EVENT - HOVER OFF\n");
						RUN_BOOSTER(hover, BOOSTER_OFF);
						DetectedCategory = true;
					}
					break;
				case KEY_BACK : // ***************** Checking Key & Touch key Event
					if(key_back != input_events[i].value) {
						key_back = input_events[i].value;
						pr_debug("[Input Booster] TOUCHKEY EVENT - %s\n", (input_events[i].value) ? "PRESS" : "RELEASE");
						RUN_BOOSTER(touchkey, (input_events[i].value) ? BOOSTER_ON : BOOSTER_OFF );
						DetectedCategory = true;
					}
					break;
				case KEY_HOMEPAGE :
					if(key_home != input_events[i].value) {
						key_home = input_events[i].value;
						pr_debug("[Input Booster] TOUCHKEY EVENT - %s\n", (input_events[i].value) ? "PRESS" : "RELEASE");
						RUN_BOOSTER(touchkey, (input_events[i].value) ? BOOSTER_ON : BOOSTER_OFF );
						DetectedCategory = true;
					}
					break;
				case KEY_RECENT :
					if(key_recent != input_events[i].value) {
						key_recent = input_events[i].value;
						pr_debug("[Input Booster] TOUCHKEY EVENT - %s\n", (input_events[i].value) ? "PRESS" : "RELEASE");
						RUN_BOOSTER(touchkey, (input_events[i].value) ? BOOSTER_ON : BOOSTER_OFF );
						DetectedCategory = true;
					}
					break;
				case KEY_VOLUMEUP :
				case KEY_VOLUMEDOWN :
				case KEY_POWER :
					pr_debug("[Input Booster] KEY EVENT - %s\n", (input_events[i].value) ? "PRESS" : "RELEASE");
					RUN_BOOSTER(key, (input_events[i].value) ? BOOSTER_ON : BOOSTER_OFF );
					DetectedCategory = true;
					break;
				case KEY_WINK :
					pr_debug("[Input Booster] key_two KEY EVENT - %s\n", (input_events[i].value) ? "PRESS" : "RELEASE");
					RUN_BOOSTER(key_two, (input_events[i].value) ? BOOSTER_ON : BOOSTER_OFF );
					DetectedCategory = true;
					break;
				default :
					break;
			}
		} else if (input_events[i].type == EV_ABS) {
			if (input_events[i].code == ABS_MT_TRACKING_ID) {
				iTouchID = i;
				if(iTouchSlot >= 0 && iTouchSlot <= MAX_EVENTS) {
					if(input_events[iTouchSlot].value < MAX_MULTI_TOUCH_EVENTS && input_events[iTouchSlot].value >= 0 && iTouchID < MAX_EVENTS) {
						if(TouchIDs[input_events[iTouchSlot].value] < 0 && input_events[iTouchID].value >= 0) {
							TouchIDs[input_events[iTouchSlot].value] = input_events[iTouchID].value;
							if((input_events[iTouchSlot].value >= 0 && touch_booster.multi_events <= 0) || (input_events[iTouchSlot].value == 0 && TouchIDs[1] < 0)) {
								touch_booster.multi_events = 0;
								pr_debug("[Input Booster] TOUCH EVENT - PRESS - ID: 0x%x, Slot: 0x%x, multi : %d\n", input_events[iTouchID].value, input_events[iTouchSlot].value, touch_booster.multi_events);
								RUN_BOOSTER(touch, BOOSTER_ON );
							} else {
								pr_debug("[Input Booster] MULTI-TOUCH EVENT - PRESS - ID: 0x%x, Slot: 0x%x, multi : %d\n", input_events[iTouchID].value, input_events[iTouchSlot].value, touch_booster.multi_events);
								touch_booster.multi_events++;
								RUN_BOOSTER(multitouch, BOOSTER_ON );
/*
								if(delayed_work_pending(&touch_booster.input_booster_timeout_work[0])) {
									int temp_hmp_boost = touch_booster.param[0].hmp_boost, temp_index = touch_booster.index;
									pr_debug("[Input Booster] ****             cancel the pending touch booster workqueue\n");
									cancel_delayed_work(&touch_booster.input_booster_timeout_work[0]);
									touch_booster.param[0].hmp_boost = multitouch_booster.param[0].hmp_boost;
									touch_booster.index = 1;
									TIMEOUT_FUNC(touch)(NULL);
									touch_booster.param[0].hmp_boost = temp_hmp_boost;
									touch_booster.index = ( temp_index >= 2 ? 1 : temp_index );
								}
*/
							}
						} else if(TouchIDs[input_events[iTouchSlot].value] >= 0 && input_events[iTouchID].value < 0) {
							TouchIDs[input_events[iTouchSlot].value] = input_events[iTouchID].value;
							if(touch_booster.multi_events <= 1) {
								pr_debug("[Input Booster] TOUCH EVENT - RELEASE - ID: 0x%x, Slot: 0x%x, multi : %d\n", input_events[iTouchID].value, input_events[iTouchSlot].value, touch_booster.multi_events);
								RUN_BOOSTER(touch, BOOSTER_OFF );
							} else {
								pr_debug("[Input Booster] MULTI-TOUCH EVENT - RELEASE - ID: 0x%x, Slot: 0x%x, multi : %d\n", input_events[iTouchID].value, input_events[iTouchSlot].value, touch_booster.multi_events);
								touch_booster.multi_events--;
								RUN_BOOSTER(multitouch, BOOSTER_OFF );
							}
						}
					}
				}
			} else if (input_events[i].code == ABS_MT_SLOT) {
				iTouchSlot = i;
#if defined(CONFIG_SOC_EXYNOS7420) // This code should be working properly in Exynos7420(Noble & Zero2) only.
				if(input_events[iTouchSlot + 1].value <