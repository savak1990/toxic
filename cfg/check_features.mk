# Check if we can use X11
CHECK_X11_LIBS = $(shell pkg-config x11 || echo -n "error")
ifneq ($(CHECK_X11_LIBS), error)
	LIBS += x11
	CFLAGS += -D_X11
endif

# Check if we want build audio support
AUDIO = $(shell if [ -z "$(DISABLE_AV)" ] || [ "$(DISABLE_AV)" = "0" ] ; then echo enabled ; else echo disabled ; fi)
ifneq ($(AUDIO), disabled)
	-include $(CFG_DIR)/av.mk
endif

# Check if we want build sound notifications support
SND_NOTIFY = $(shell if [ -z "$(DISABLE_SOUND_NOTIFY)" ] || [ "$(DISABLE_SOUND_NOTIFY)" = "0" ] ; then echo enabled ; else echo disabled ; fi)
ifneq ($(SND_NOTIFY), disabled)
	-include $(CFG_DIR)/sound_notifications.mk
endif

# Check if we want build desktop notifications support
DESK_NOTIFY = $(shell if [ -z "$(DISABLE_DESKTOP_NOTIFY)" ] || [ "$(DISABLE_DESKTOP_NOTIFY)" = "0" ] ; then echo enabled ; else echo disabled ; fi)
ifneq ($(DESK_NOTIFY), disabled)
	-include $(CFG_DIR)/desktop_notifications.mk
endif

# Check if we can build Toxic
CHECK_LIBS = $(shell pkg-config $(LIBS) || echo -n "error")
ifneq ($(CHECK_LIBS), error)
	CFLAGS += $(shell pkg-config --cflags $(LIBS))
	LDFLAGS += $(shell pkg-config --libs $(LIBS))
else
ifneq ($(MAKECMDGOALS), clean)
MISSING_LIBS = $(shell for lib in $(LIBS) ; do if ! pkg-config $$lib ; then echo $$lib ; fi ; done)
$(warning ERROR -- Cannot compile Toxic)
$(warning ERROR -- You need these libraries)
$(warning ERROR -- $(MISSING_LIBS))
$(error ERROR)
endif
endif
