%-protocol.c : $(protocoldir)/%.xml
	$(AM_V_GEN)$(wayland_scanner) code < $< > $@

%-server-protocol.h : $(protocoldir)/%.xml
	$(AM_V_GEN)$(wayland_scanner) server-header < $< > $@

%-client-protocol.h : $(protocoldir)/%.xml
	$(AM_V_GEN)$(wayland_scanner) client-header < $< > $@
