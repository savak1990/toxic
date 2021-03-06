MISC_DIR = ../misc
DOC_DIR = ../doc
SND_DIR = ../sounds
DATAFILES = DHTnodes toxic.conf.example
MANFILES = toxic.1 toxic.conf.5
SNDFILES = ContactLogsIn.wav ContactLogsOut.wav Error.wav IncomingCall.wav
SNDFILES += LogIn.wav LogOut.wav NewMessage.wav OutgoingCall.wav
SNDFILES += TransferComplete.wav TransferPending.wav

install: toxic
	mkdir -p $(abspath $(DESTDIR)/$(BINDIR))
	mkdir -p $(abspath $(DESTDIR)/$(DATADIR))
	mkdir -p $(abspath $(DESTDIR)/$(DATADIR))/sounds
	mkdir -p $(abspath $(DESTDIR)/$(MANDIR))
	@echo "Installing toxic executable"
	@install -m 0755 toxic $(abspath $(DESTDIR)/$(BINDIR))
	@echo "Installing data files"
	@for f in $(DATAFILES) ; do \
		install -m 0644 $(MISC_DIR)/$$f $(abspath $(DESTDIR)/$(DATADIR)) ;\
		file=$(abspath $(DESTDIR)/$(DATADIR))/$$f ;\
		sed -i'' -e 's:__DATADIR__:'$(abspath $(DATADIR))':g' $$file ;\
	done
	@for f in $(SNDFILES) ; do \
		install -m 0644 $(SND_DIR)/$$f $(abspath $(DESTDIR)/$(DATADIR))/sounds ;\
	done
	@echo "Installing man pages"
	@for f in $(MANFILES) ; do \
		section=$(abspath $(DESTDIR)/$(MANDIR))/man`echo $$f | rev | cut -d "." -f 1` ;\
		file=$$section/$$f ;\
		mkdir -p $$section ;\
		install -m 0644 $(DOC_DIR)/$$f $$file ;\
		sed -i'' -e 's:__VERSION__:'$(VERSION)':g' $$file ;\
		sed -i'' -e 's:__DATADIR__:'$(abspath $(DATADIR))':g' $$file ;\
		gzip -f -9 $$file ;\
	done

.PHONY: install
