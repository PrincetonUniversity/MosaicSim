###########################################################
# DC script to perform power analysis
###########################################################

set POWER_DIR ${DESIGN_NAME}/power
file mkdir ${POWER_DIR}

set initial_search_path [get_app_var search_path]

foreach opp $operating_points {
    set dir [lindex $opp 0]
    set clk_period [lindex $opp 1]

    # Set nominal library path before loading the mapped design
    set DB_PATH $TECH_PATH/db/$dir
    set ADDITIONAL_SEARCH_PATH $DB_PATH

    set LINK_LIB_FILES ""
    foreach f [ls $DB_PATH/*.db] {
	set name [file tail $f]
	append LINK_LIB_FILES " $name"
    }
    set_app_var search_path ". dc_src ${ADDITIONAL_SEARCH_PATH} $initial_search_path"
    set_app_var synthetic_library "dw_foundation.sldb"
    set_app_var link_library "* $LINK_LIB_FILES $synthetic_library"

    #Setup SAIF Name Mapping Database
    saif_map -start
    set power_preserve_rtl_hier_names true

    # Read design
    read_ddc ${RESULTS_DIR}/${TOP_NAME}.ddc

    link

    create_clock -name clk [get_ports clk] -period ${clk_period}

    # Report power with standard switching activity
    puts "\n ** REPORT POWER $dir $POWER_DIR **\n" 
    report_power -analysis_effort high -verbose -nosplit > ${POWER_DIR}/power_std_$dir.rpt

    # if { [file exists "../back_sc.saif" ] == 1} {
	# reset_switching_activity
	# read_saif -auto_map_names -input ../back_sc.saif -instance_name nmf_multt0
	# puts "\n ** Reading SAIF backannotation (RTL) **\n"
	# report_power -analysis_effort high -verbose -nosplit > ${POWER_DIR}/power_sc_$dir.rpt
	report_saif -hier
    # }

    # Report timing -> check clock cycle meets timing
    puts "\n ** REPORT TIMING $dir $POWER_DIR **\n" 
    report_timing -transition_time -nets -attributes -nosplit > ${POWER_DIR}/timing_$dir.rpt
}

exit

# # TODO: run RTL and GL cosimulations with SystemC testbench and extract switching activity
# if { [file exists "../incisive/back_rtl.saif" ] == 1} {
#    reset_switching_activity
#    read_saif -auto_map_names -input ../incisive/back_rtl.saif -instance_name ${INSTANCE_NAME}
#    puts "\n ** Reading SAIF backannotation (RTL) **\n"
#    report_power -analysis_effort high -verbose -nosplit > ${POWER_DIR}/power_rtl.rpt
#    report_saif -hier
# }
# if { [file exists "../incisive/back_gl.saif" ] == 1} {
#    reset_switching_activity
#    read_saif -auto_map_names -input ../incisive/back_gl.saif -instance_name ${INSTANCE_NAME}
#    puts "\n ** Reading SAIF backannotation (gate-level) **\n"
#    report_power -analysis_effort high -verbose -nosplit > ${POWER_DIR}/power_gl.rpt
#    report_saif -hier
# }
