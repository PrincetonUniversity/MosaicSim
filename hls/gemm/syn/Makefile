# Copyright 2018 Columbia University, SLD Group

MEM_SPECDIR = ../plm/

MEM_OUTPUTDIR = ../plm/output

MEM_GENERATOR =../../utils/memgen/memgen.py 

TECH_LIBRARY = ../../tech/cmos32soi/mem

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
