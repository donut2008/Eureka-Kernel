device;
	void            (*handler)(struct ib_event_handler *, struct ib_event *);
	struct list_head  list;
};

#define INIT_IB_EVENT_HANDLER(_ptr, _device, _handler)		\
	do {							\
		(_ptr)->device  = _dev