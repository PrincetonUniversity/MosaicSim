# Copyright 2018 Columbia University, SLD Group

MEM_SPECDIR = ../plm/

MEM_OUTPUTDIR = ../plm/output

MEM_GENERATOR = ../../utils/memgen/memgen.py 

TECH_LIBRARY = ../../tech/cmos32soi/mem

RTL_OUT = $(HLS_ROOT)/tech/$(TECH)/acc/$(ACCELERATOR)

TARGET_NAME = sdp

# Disable timing checks
BDW_VLOGSIM_ARGS = -NOTIMINGCHECKS

# Uncomment for Insight
BDW_DEBUG = 1

saySimPassed:
	@bdw_sim_pass

-include Makefile.prj

generate_plm: ${MEM_SPECDIR}/spec.txt
	@rm -rf memlib/* ${MEM_OUTPUTDIR}/*
	@${MEM_GENERATOR} ${TECH_LIBRARY} ${MEM_SPECDIR}/spec.txt ${MEM_OUTPUTDIR} &>/dev/null
	@bdw_memgen memlib/*.bdm &> /dev/null
	@echo "Memories generated successfully"

Makefile.prj : project.tcl ../../common/stratus/project.tcl
	@bdw_makegen

# -include Makefile.prj

install: Makefile.prj hls_all
	@mkdir -p $(RTL_OUT)
	@for dir in $$(ls bdw_work/modules/$(TARGET_NAME)); do \
		indir=bdw_work/modules/$(TARGET_NAME)/$$dir; \
		infile=$$indir/$(TARGET_NAME)_rtl.v; \
		if test -e $$infile; then \
			cfg=$${dir,,}; \
			outdir=$(RTL_OUT)/$(TARGET_NAME)_$$cfg; \
			outfile=$$outdir/$(TARGET_NAME)_$$cfg.v; \
			mkdir -p $$outdir; \
			if [ $$infile -nt $$outfile ]; then \
				cp $$infile $$outfile; \
				rtl_parts=$$(rm -rf tmp-vsim; vlib tmp-vsim; vlog -work tmp-vsim -quiet $$infile; vsim -quiet -c tmp-vsim.$(TARGET_NAME) > log 2> /dev/null; cat log | grep Error | grep "not defined" | cut -f 6 -d " " | sed "s/'//g" | uniq; rm -rf tmp-vsim log); \
				for p in $$rtl_parts; do \
					pfile=$$indir/v_rtl/$$p\.v; \
					if test -e $$pfile; then \
						cp $$pfile $$outdir; \
					fi; \
				done; \
				sed -i "s/module $(TARGET_NAME)/module $(TARGET_NAME)_$$cfg/g" $$outfile; \
			fi; \
		fi; \
	done

# Clean out undesirable junk files from the project directory
# Uses the automatically created clean_all target from Makefile.prj
clean: clean_all clean_libs
	@rm -f transcript vsim* *.wlf data.out
	@rm -f *~ *.bak *.BAK
	@rm -rf work debussy* vfast*
	@rm -f Makefile.prj
	@rm -f msg_help.html
	@rm -rf core *.jou
	@rm -rf core.*
	@rm -rf .stack.*
	@rm -rf *irun.history*
	@rm -rf *.pro *.pro.user
	@rm -rf INCA_libs
	@rm -rf *.log
	@rm -rf irun.key
	@rm -rf waves.shm
	@rm -rf .simvision
	@rm -rf .project.*
	@rm -rf .stratus*
	@rm -rf .syn.qws

CLEAN: clean
