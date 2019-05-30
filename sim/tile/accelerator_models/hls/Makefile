
HLS_ROOT = $(PWD)
TECHLIB = cmos32soi
POW_TECHLIB = cmos32soi

ACCELERATORS_PATH		= $(HLS_ROOT)
ACCELERATORS			= $(filter-out common, $(shell ls -d $(ACCELERATORS_PATH)/*/ | awk -F/ '{print $$(NF-1)}'))
ACCELERATORS-wdir		= $(addsuffix -wdir, $(ACCELERATORS))
ACCELERATORS-hls		= $(addsuffix -hls, $(ACCELERATORS))
ACCELERATORS-clean		= $(addsuffix -clean, $(ACCELERATORS))
ACCELERATORS-distclean		= $(addsuffix -distclean, $(ACCELERATORS))
ACCELERATORS-sim		= $(addsuffix -sim, $(ACCELERATORS))
ACCELERATORS-plot		= $(addsuffix -plot, $(ACCELERATORS))

ACCELERATORS-asic-wdir		= $(addsuffix -asic-wdir, $(ACCELERATORS))
ACCELERATORS-pow-wdir		= $(addsuffix -pow-wdir, $(ACCELERATORS))
ACCELERATORS-asic-hls		= $(addsuffix -asic-hls, $(ACCELERATORS))
ACCELERATORS-power-estimation	= $(addsuffix -power-estimation, $(ACCELERATORS))

ACCELERATORS-driver		= $(addsuffix -driver, $(ACCELERATORS))
ACCELERATORS-driver-clean	= $(addsuffix -driver-clean, $(ACCELERATORS))
ACCELERATORS-app		= $(addsuffix -app, $(ACCELERATORS))
ACCELERATORS-app-clean		= $(addsuffix -app-clean, $(ACCELERATORS))
ACCELERATORS-barec		= $(addsuffix -barec, $(ACCELERATORS))
ACCELERATORS-barec-clean	= $(addsuffix -barec-clean, $(ACCELERATORS))


#
# List available accelerators
#
print-available-accelerators:
	$(QUIET_INFO)echo "Available accelerators: $(ACCELERATORS)"

.PHONY: print-available-accelerators


#
# Create working directories
#
$(ACCELERATORS-wdir):
	$(QUIET_MKDIR)mkdir -p $(ACCELERATORS_PATH)/$(@:-wdir=)/hls-work-$(TECHLIB)
	@cd $(ACCELERATORS_PATH)/$(@:-wdir=)/hls-work-$(TECHLIB); \
	if test ! -e project.tcl; then \
		ln -s ../syn/project.tcl; \
	fi; \
	if test ! -e Makefile; then \
		ln -s ../syn/Makefile; \
	fi;

$(ACCELERATORS-asic-wdir):
	$(QUIET_MKDIR)mkdir -p $(ACCELERATORS_PATH)/$(@:-asic-wdir=)/hls-work-$(POW_TECHLIB)
	@cd $(ACCELERATORS_PATH)/$(@:-asic-wdir=)/hls-work-$(POW_TECHLIB); \
	if test ! -e project.tcl; then \
		ln -s ../syn/project.tcl; \
	fi; \
	if test ! -e Makefile; then \
		ln -s ../syn/Makefile; \
	fi;

$(ACCELERATORS-pow-wdir):
	$(QUIET_MKDIR)mkdir -p $(ACCELERATORS_PATH)/$(@:-pow-wdir=)/synopsys-work-$(POW_TECHLIB)
	@cd $(ACCELERATORS_PATH)/$(@:-pow-wdir=)/synopsys-work-$(POW_TECHLIB); \
	if test ! -e dc_synthesis.tcl; then \
		ln -s ../synopsys/dc_synthesis.tcl; \
	fi; \
	if test ! -e dc_power.tcl; then \
		ln -s ../synopsys/dc_power.tcl; \
	fi; \
	if test ! -e dc_constraints.tcl; then \
		ln -s ../synopsys/dc_constraints.tcl; \
	fi; \
	if test ! -e Makefile; then \
		ln -s ../synopsys/Makefile; \
	fi;

.PHONY: $(ACCELERATORS-wdir) $(ACCELERATORS-asic-wdir) $(ACCELERATORS-pow-wdir)

#
# HLS
#
$(ACCELERATORS-hls): %-hls : %-wdir
	$(QUIET_MAKE)ACCELERATOR=$(@:-hls=) TECH=$(TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-hls=)/hls-work-$(TECHLIB) generate_plm | tee $(@:-hls=)_memgen.log
	$(QUIET_INFO)echo "Running HLS for available implementations of $(@:-hls=)"
	@ACCELERATOR=$(@:-hls=) TECH=$(TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-hls=)/hls-work-$(TECHLIB) hls_all | tee $(@:-hls=)_hls.log
	$(QUIET_INFO)echo "Installing available implementations for $(@:-hls=) to $(HLS_ROOT)/tech/$(TECHLIB)/acc/$(@:-hls=)"
	@ACCELERATOR=$(@:-hls=) TECH=$(TECHLIB) HLS_ROOT=$(HLS_ROOT) make --quiet -C $(ACCELERATORS_PATH)/$(@:-hls=)/hls-work-$(TECHLIB) install
	@mkdir -p $(HLS_ROOT)/tech/$(TECHLIB)/memgen/$(@:-hls=)
	@cp -r  $(HLS_ROOT)/$(@:-hls=)/plm/output/*  $(HLS_ROOT)/tech/$(TECHLIB)/memgen/$(@:-hls=)
	@if ! test -e $(HLS_ROOT)/tech/$(TECHLIB)/acc/installed.log; then \
		touch $(HLS_ROOT)/tech/$(TECHLIB)/acc/installed.log; \
	fi;
	@sed -i '/$(@:-hls=)/d' $(HLS_ROOT)/tech/$(TECHLIB)/acc/installed.log
	@echo "$(@:-hls=)" >> $(HLS_ROOT)/tech/$(TECHLIB)/acc/installed.log


$(ACCELERATORS-asic-hls): %-asic-hls : %-asic-wdir
	$(QUIET_MAKE)ACCELERATOR=$(@:-asic-hls=) TECH=$(POW_TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-asic-hls=)/hls-work-$(POW_TECHLIB) generate_plm | tee $(@:-asic-hls=)_memgen.log
	$(QUIET_INFO)echo "Running HLS for available implementations of $(@:-asic-hls=)"
	@ACCELERATOR=$(@:-asic-hls=) TECH=$(POW_TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-asic-hls=)/hls-work-$(POW_TECHLIB) hls_all | tee $(@:-asic-hls=)_hls.log
	$(QUIET_INFO)echo "Installing available implementations for $(@:-asic-hls=) to $(HLS_ROOT)/tech/$(POW_TECHLIB)/acc/$(@:-asic-hls=)"
	@ACCELERATOR=$(@:-asic-hls=) TECH=$(POW_TECHLIB) HLS_ROOT=$(HLS_ROOT) make --quiet -C $(ACCELERATORS_PATH)/$(@:-asic-hls=)/hls-work-$(POW_TECHLIB) install
	@mkdir -p $(HLS_ROOT)/tech/$(TECHLIB)/memgen/$(@:-asic-hls=)
	@cp -r  $(HLS_ROOT)/$(@:-asic-hls=)/plm/output/*  $(HLS_ROOT)/tech/$(TECHLIB)/memgen/$(@:-asic-hls=)
	@sed -i '/$(@:-asic-hls=)/d' $(HLS_ROOT)/tech/$(POW_TECHLIB)/acc/installed.log
	@echo "$(@:-asic-hls=)" >> $(HLS_ROOT)/tech/$(POW_TECHLIB)/acc/installed.log

.PHONY: $(ACCELERATORS-hls) $(ACCELERATORS-asic-hls)

accelerators: $(ACCELERATORS-hls)

.PHONY: accelerators


#
# Logic Synthesis for power estimation
#

# $(ACCELERATORS-power-estimation): %-power-estimation : %-asic-hls %-pow-wdir
$(ACCELERATORS-power-estimation): %-power-estimation : %-pow-wdir
	$(QUIET_RUN)
	@for f in $$(ls -d $(HLS_ROOT)/tech/$(POW_TECHLIB)/acc/$(@:-power-estimation=)/*/); do \
		hls_cfg=$$(basename $$f); \
		echo $(SPACES)"INFO: Running logic synthesis for $$hls_cfg"; \
		ACCELERATOR=$(@:-power-estimation=) TECH=$(POW_TECHLIB) HLS_ROOT=$(HLS_ROOT) DESIGN_NAME=$$hls_cfg make -C $(ACCELERATORS_PATH)/$(@:-power-estimation=)/synopsys-work-$(POW_TECHLIB); \
	done;

.PHONY: $(ACCELERATORS-power-estimation)

accelerators-power-estimation: $(ACCELERATORS-power-estimation)

.PHONY: accelerators-power-estimation

#
# Simulation
#
$(ACCELERATORS-sim): %-sim : %-wdir
	$(QUIET_RUN)ACCELERATOR=$(@:-sim=) TECH=$(TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-sim=)/hls-work-$(TECHLIB) sim_all | tee $(@:-sim=)_sim.log

$(ACCELERATORS-plot): %-plot : %-wdir
	$(QUIET_RUN)ACCELERATOR=$(@:-plot=) TECH=$(TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-plot=)/hls-work-$(TECHLIB) plot

.PHONY: $(ACCELERATORS-sim) $(ACCELERATORS-plot)

#
# Clean
#
$(ACCELERATORS-clean): %-clean : %-wdir
	$(QUIET_CLEAN)ACCELERATOR=$(@:-clean=) TECH=$(TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-clean=)/hls-work-$(TECHLIB) clean
	@ACCELERATOR=$(@:-clean=) TECH=$(POW_TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-clean=)/synopsys-work-$(POW_TECHLIB) clean
	@$(RM) $(@:-clean=)*.log

$(ACCELERATORS-distclean): %-distclean : %-wdir
	$(QUIET_CLEAN) ACCELERATOR=$(@:-distclean=) TECH=$(POW_TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-distclean=)/synopsys-work-$(POW_TECHLIB) distclean
	@ACCELERATOR=$(@:-distclean=) TECH=$(TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-distclean=)/hls-work-$(TECHLIB) distclean
	@sed -i '/$(@:-distclean=)/d' $(HLS_ROOT)/tech/$(TECHLIB)/acc/installed.log
	@ACCELERATOR=$(@:-distclean=) TECH=$(POW_TECHLIB) HLS_ROOT=$(HLS_ROOT) make -C $(ACCELERATORS_PATH)/$(@:-distclean=)/hls-work-$(POW_TECHLIB) distclean
	@sed -i '/$(@:-distclean=)/d' $(HLS_ROOT)/tech/$(POW_TECHLIB)/acc/installed.log
	@$(RM) $(@:-distclean=)*.log

.PHONY: $(ACCELERATORS-clean) $(ACCELERATORS-distclean)

accelerators-clean: $(ACCELERATORS-clean)

accelerators-distclean: $(ACCELERATORS-distclean)

.PHONY: accelerators-clean accelerators-distclean
