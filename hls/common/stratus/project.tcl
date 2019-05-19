# Copyright 2018 Columbia University, SLD Group

#
# Technology Libraries
#

set DEC_ACC_ROOT "../.."

#
# Setup technology and include behavioral models and/or libraries
#
set fpga_techs [list "virtex7"]
set asic_techs [list "cmos32soi"]

# set TECH to virtex7 if unset
if {![info exists ::env(TECH)]} {
    set TECH "cmos32soi"
} else { 
    set TECH $::env(TECH)
    if {[lsearch $fpga_techs $TECH] < 0} {
	if {[lsearch $asic_techs $TECH] < 0} {
	    set TECH "cmos32soi"
	}
    }
}

set TECH_PATH "$DEC_ACC_ROOT/tech/$TECH"


if {[lsearch $fpga_techs $TECH] >= 0} {

    set VIVADO $::env(XILINX_VIVADO)

    set_attr fpga_use_dsp on
    set_attr fpga_tool "vivado"
    set_attr fpga_part "xc7v2000tflg1925-2"

    set_attr verilog_files "../../tech/virtex7/mem/*.v"
    set_attr verilog_files "$VIVADO/ids_lite/ISE/verilog/src/glbl.v"
    set_attr verilog_files "$VIVADO/ids_lite/ISE/verilog/src/unisims/RAMB16_S*.v"

    set TECH_IS_XILINX 1
}
if {[lsearch $asic_techs $TECH] >= 0} {

    set_attr verilog_files "$TECH_PATH/verilog/*v $TECH_PATH/mem/*v"
    set LIB_PATH "$TECH_PATH/lib"
    set LIB_NAME "1p0v/ibm32soi_hvt_1p0v.lib"
    use_tech_lib "$LIB_PATH/$LIB_NAME"

    set TECH_IS_XILINX 0

    use_hls_lib "[get_install_path]/share/stratus/cynware/cynw_cm_float"
}

#
# Global synthesis attributes
#

set_attr message_detail           2
set_attr default_input_delay      0.1
set_attr default_protocol         false
set_attr inline_partial_constants true
set_attr output_style_reset_all   true
set_attr lsb_trimming             true

#
# Global project includes
#

# Includes for common data types for NMF

set NMF_HDRS_PATH "-I../../common/nmf-data"

# Includes for common data types for WAMI

set WAMI_HDRS_PATH "-I../../common/wami-data"

# Includes for template synthesis for ESP

set ESP_HDRS_PATH "-I../../esp-accelerator-templates"

# Includes for memories (generated wit memlib)

set MEM_HDRS_PATH "-I./memlib -I./memlib/c_parts"

set INCLUDES "$ESP_HDRS_PATH $NMF_HDRS_PATH $WAMI_HDRS_PATH $MEM_HDRS_PATH -I../src"

#
# High-Level Synthesis Options
#

set_attr hls_cc_options "$INCLUDES"

#
# Compiler and Simulator Options
#

set_systemc_options -gcc 4.1

set_attr cc_options "$INCLUDES"

use_systemc_simulator incisive
set_attr end_of_sim_command "make saySimPassed"

