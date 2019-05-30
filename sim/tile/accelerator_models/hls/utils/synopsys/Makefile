### Some pretty aliases ###

# Define V=1 for a more verbose compilation
ifndef V
	QUIET_AR            = @echo '   ' AR $@;
	QUIET_AS            = @echo '   ' AS $@;
	QUIET_BUILD         = @echo '   ' BUILD $@;
	QUIET_CHMOD         = @echo '   ' CHMOD $@;
	QUIET_CC            = @echo '   ' CC $@;
	QUIET_CXX           = @echo '   ' CXX $@;
	QUIET_OBJCP         = @echo '   ' OBJCP $@;
	QUIET_CHECKPATCH    = @echo '   ' CHECKPATCH $(subst .o,.c,$@);
	QUIET_CHECK         = @echo '   ' CHECK $(subst .o,.c,$@);
	QUIET_LINK          = @echo '   ' LINK $@;
	QUIET_CP            = @echo '   ' CP $@;
	QUIET_RM            = @echo '   ' RM $@;
	QUIET_MKDIR         = @echo '   ' MKDIR $@;
	QUIET_MAKE          = @echo '   ' MAKE $@;
	QUIET_INFO          = @echo -n '   ' INFO '';
	QUIET_DIFF          = @echo -n '   ' DIFF '';
	QUIET_RUN           = @echo '   ' RUN $@;
	QUIET_CLEAN         = @echo '   ' CLEAN $@;
endif
	SPACES              = "    "
	SPACING             = echo -n "    ";

RM  = rm -rf

TARGET_NAME = $(ACCELERATOR)

# Memory wrappers
MEMGEN = $(HLS_ROOT)/utils/memgen/memgen.py
MEMTECH = $(HLS_ROOT)/tech/$(TECH)/mem
MEMGEN_OUT = $(HLS_ROOT)/tech/$(TECH)/memgen/$(TARGET_NAME)

RTL_OUT = $(HLS_ROOT)/tech/$(TECH)/acc/$(ACCELERATOR)

RTL_PATH = $(RTL_OUT)/$(DESIGN_NAME)
RTL_SRCS = $(wildcard $(RTL_PATH)/*.v) $(wildcard $(RTL_PATH)/*.sv)

all: $(RTL_PATH)/power_std.rpt

$(DESIGN_NAME)/out/$(DESIGN_NAME).ddc: $(RTL_SRCS)
	$(QUIET_MKDIR) mkdir -p $(DESIGN_NAME)
	$(QUIET_MAKE) MEMGEN_OUT=$(MEMGEN_OUT) RTL_PATH=$(RTL_PATH) TECH=${TECH} DESIGN_NAME=${DESIGN_NAME} HLS_ROOT=${HLS_ROOT} dc_shell -64 -no_log -f ./dc_synthesis.tcl 2>&1 | tee $(DESIGN_NAME)/synthesis.log

$(RTL_OUT)/$(DESIGN_NAME)/power_std.rpt: $(DESIGN_NAME)/out/$(DESIGN_NAME).ddc
	$(QUIET_MAKE) MEMGEN_OUT=$(MEMGEN_OUT) RTL_PATH=$(RTL_PATH) TECH=${TECH} DESIGN_NAME=${DESIGN_NAME} HLS_ROOT=${HLS_ROOT} dc_shell -64 -no_log -f ./dc_power.tcl 2>&1 | tee $(DESIGN_NAME)/power.log
	@echo "(name) (V) (ps) (mW)" > $@
	@rm -f $(DESIGN_NAME)/power/summary.rpt
	@for power_rpt in $$(ls $(DESIGN_NAME)/power/power_std*); do \
		file_name=$$(basename $$power_rpt); \
		opp=$$(echo $$file_name | sed 's/power_std_//g' | sed 's/.rpt//g'); \
		vdd=$$(cat $$power_rpt | grep "Global Operating Voltage" | cut -d " " -f 5); \
		clk=$$(cat $(DESIGN_NAME)/power/timing_$$opp.rpt  | grep "clock" | grep "rise edge" | grep -v "\s0.00" | sed 's/\s\+/ /g' | uniq | cut -f 6 -d " "); \
		pow=$$(tail -n 2 $$power_rpt | head -n 1 | sed "s/\s\+/ /g" | cut -f 8 -d " "); \
		echo "$$opp $$vdd $$clk $$pow" >> $(DESIGN_NAME)/power/summary.rpt; \
	done;
	@sort --rev -k2 -n $(DESIGN_NAME)/power/summary.rpt >> $@

clean:
	$(QUIET_CLEAN) $(RM) *.log *.svf

distclean:
	$(QUIET_CLEAN) $(RM) $(TARGET_NAME)_* $(RTL_OUT)/$(DESIGN_NAME)/power_std.rpt

.PHONY: all clean distclean
