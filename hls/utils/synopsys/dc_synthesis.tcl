###########################################################
# DC script to perform logic synthesis
###########################################################

set DB_PATH $TECH_PATH/db/$opp_nominal
set ADDITIONAL_SEARCH_PATH $DB_PATH

set RTL_SOURCE_FILES [ls $RTL_PATH/*.v $MEMGEN_OUT/*.v $TECH_PATH/mem/*.v]

###############
# Library setup
###############

set_app_var alib_library_analysis_path "alibCenter"

set TARGET_LIBRARY_FILES $TARGET_LIBRARY.db

set LINK_LIB_FILES ""
foreach f [ls $DB_PATH/*.db] {
    set name [file tail $f]
    append LINK_LIB_FILES " $name"
}

set_app_var search_path ". dc_src ${ADDITIONAL_SEARCH_PATH} $search_path"
set_app_var target_library ${TARGET_LIBRARY_FILES}
set_app_var synthetic_library "dw_foundation.sldb"
set_app_var link_library "* $LINK_LIB_FILES $synthetic_library"

#################################################################################
# Setup SAIF Name Mapping Database (necessary for power analysis)
#################################################################################
saif_map -start
set power_preserve_rtl_hier_names true

#################################################################################
# Read in the RTL source files.
#################################################################################
define_design_lib WORK -path ./${RESULTS_DIR}/WORK
analyze -format verilog ${RTL_SOURCE_FILES}
elaborate "${TOP_NAME}"
link

#################################################################################
# Apply Logical Design Constraints
#################################################################################
#local constraints override the default ones
if { [file exists ./dc_constraints.tcl] == 1} {
    source -echo -verbose ./dc_constraints.tcl
} else {
    #default synthesis constraints (library time unit is 1ns)
    set timing_input_port_default_clock TRUE
    create_clock -name clk [get_ports clk] -period ${CLOCK_PERIOD}
    set_clock_uncertainty ${estimated_clock_uncertainty} clk
    set_input_delay ${estimated_input_delay} -clock clk [all_inputs]
    set_dont_touch_network clk
    set ports_clock_root [filter_collection [get_attribute [get_clocks] sources] object_class==port]
    set non_clock_inputs [remove_from_collection [all_inputs] ${ports_clock_root}]
    set_false_path -from [get_ports rst]
    set_ideal_network -no_prop [get_ports rst]
    set_input_delay ${estimated_clock_input_delay} -clock clk [remove_from_collection [all_inputs] ${ports_clock_root}]
    set_output_delay ${estimated_output_delay} -reference_pin clk [all_outputs]
    set_driving_cell -no_design_rule -max -lib_cell ${default_driving_cell} -library ${TARGET_LIBRARY} ${non_clock_inputs}
    set_driving_cell -no_design_rule -min -lib_cell ${default_driving_cell} -library ${TARGET_LIBRARY} ${non_clock_inputs}
    set_drive 0 ${ports_clock_root}
    set_max_transition 40 [current_design]
    set_max_fanout 8 ${TOP_NAME}
}

#################################################################################
# Create Default Path Groups
#
# Separating these paths can help improve optimization.
# Remove these path group settings if user path groups have already been defined.
#################################################################################
set ports_clock_root [filter_collection [get_attribute [get_clocks] sources] object_class==port]
group_path -name REGOUT -to [all_outputs]
group_path -name REGIN -from [remove_from_collection [all_inputs] ${ports_clock_root}]
group_path -name FEEDTHROUGH -from [remove_from_collection [all_inputs] ${ports_clock_root}] -to [all_outputs]

#################################################################################
# Prevent assignment statements in the Verilog netlist.
#################################################################################
set_fix_multiple_port_nets -all -buffer_constants

#################################################################################
# Check for Design Errors
#################################################################################
check_design -summary
check_design > ${REPORTS_DIR}/check.rpt

#################################################################################
# Compile the Design
#################################################################################
#set_fix_hold [all_clocks]
#compile_ultra -retime -no_autoungroup
compile_ultra -no_autoungroup

#################################################################################
# High-effort area optimization
#
# optimize_netlist -area command, was introduced in I-2013.12 release to improve
# area of gate-level netlists. The command performs monotonic gate-to-gate
# optimization on mapped designs, thus improving area without degrading timing or
# leakage.
#################################################################################
optimize_netlist -area

#################################################################################
# Write Out Final Design and Reports
#        .ddc:   Recommended binary format used for subsequent Design Compiler sessions
#        .v  :   Verilog netlist for ASCII flow (Formality, PrimeTime, VCS)
#################################################################################
change_names -rules verilog -hierarchy
write -format verilog -hierarchy -output ${RESULTS_DIR}/${TOP_NAME}.v
write -format ddc     -hierarchy -output ${RESULTS_DIR}/${TOP_NAME}.ddc
write_sdf ${RESULTS_DIR}/${TOP_NAME}.sdf

#################################################################################
# Generate Final Reports
#################################################################################
#timing
report_timing -transition_time -nets -attributes -nosplit > ${REPORTS_DIR}/timing.rpt
report_timing -delay max -nworst 1 -max_paths 10 -path end -nosplit -unique -sort_by slack > ${REPORTS_DIR}/timing.setup.rpt
report_timing -delay min -nworst 1 -max_paths 10 -path short -nosplit -unique -sort_by slack > ${REPORTS_DIR}/timing.hold.rpt
#area
report_area -physical -nosplit > ${REPORTS_DIR}/area.rpt
report_area -physical -hierarchy -nosplit > ${REPORTS_DIR}/area.hierarchy.rpt

exit
