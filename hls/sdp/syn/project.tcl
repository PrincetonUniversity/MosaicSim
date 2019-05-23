# Copyright 2018 Columbia University, SLD Group

#
# Source the common configurations
#

source ../../common/stratus/project.tcl

#
# Set the library for the memories
#

use_hls_lib "./memlib"

#
# Set the target clock and reset period
#

if {$TECH eq "virtex7"} {
    # Library is in ns, but simulation uses ps!                                                        
    set CLOCK_PERIOD 10.0
    set SIM_CLOCK_PERIOD 10000.0
    set_attr default_input_delay      0.1
}
if {$TECH eq "cmos32soi"} {
    set CLOCK_PERIOD 1000.0
    set SIM_CLOCK_PERIOD 1000.0
    set_attr default_input_delay      100.0
}

set SIM_RESET_PERIOD [expr $SIM_CLOCK_PERIOD * 30]

#
# Set common options for all configurations
#

set PRINT no
set SCHED_ASAP no

# if FIXED_POINT
#set COMMON_HLS_FLAGS \
#    "-DFIXED_POINT --clock_period=$CLOCK_PERIOD --prints=$PRINT --sched_asap=$SCHED_ASAP"
#set COMMON_CFG_FLAGS \
#    "-DFIXED_POINT -DCLOCK_PERIOD=$SIM_CLOCK_PERIOD -DRESET_PERIOD=$SIM_RESET_PERIOD"
# if FLOAT_POINT
set COMMON_HLS_FLAGS \
    "-DFLOAT_POINT --clock_period=$CLOCK_PERIOD --prints=$PRINT --sched_asap=$SCHED_ASAP"
set COMMON_CFG_FLAGS \
    "-DFLOAT_POINT -DCLOCK_PERIOD=$SIM_CLOCK_PERIOD -DRESET_PERIOD=$SIM_RESET_PERIOD"

#
# Testbench or system level modules
#

define_system_module ../tb/sdp.c
define_system_module tb ../tb/system.cpp ../tb/sc_main.cpp

#
# System level modules to be synthesized
#

define_hls_module sdp ../src/sdp.cpp

#
# TB configuration
#

set INPUT_PATH  "../input"
set OUTPUT_PATH "../output"

set TESTBENCHES ""
append TESTBENCHES "test1 test2 test3 test4 test5 "

#
# DSE configurations
#

set DMA_CHUNKS ""
append DMA_CHUNKS "512 "

set DMA_WIDTH "64"

#set NUM_PORTS ""
#append NUM_PORTS "1 "
#append NUM_PORTS "2 " 
#append NUM_PORTS "4 "
#append NUM_PORTS "8 "

set BASIC_OPTIONS "$COMMON_HLS_FLAGS"

set_attr split_multiply 32
set_attr split_add 32


#
# Generating sim/system configs
#

foreach chk $DMA_CHUNKS {

    # foreach pts $NUM_PORTS {

	foreach dma $DMA_WIDTH {

	    set conf "CHK$chk\_PP$pts\_DMA$dma"

	    define_io_config * IOCFG_$conf -DDMA_WIDTH=$dma -DDMA_CHUNK=$chk $COMMON_CFG_FLAGS

	    define_hls_config sdp BASIC_$conf -io_config IOCFG_$conf $COMMON_HLS_FLAGS

	    define_system_config tb TESTBENCH_$conf -io_config IOCFG_$conf

	    foreach tb $TESTBENCHES {

		set ARGV ""
		append ARGV "$INPUT_PATH/$tb\_1.txt ";  # argv[1]
		append ARGV "$INPUT_PATH/$tb\_2.txt ";  # argv[2]
		append ARGV "$OUTPUT_PATH/$tb.txt ";    # argv[3]

		# Behavioral simulation

		define_sim_config "BEHAV_$conf\_$tb" "sdp BEH" "tb TESTBENCH_$conf" \
		    -io_config IOCFG_$conf -argv $ARGV

		# RTL Verilog simulation
		if {$TECH_IS_XILINX == 1} {                            
		    define_sim_config "BASIC_$conf\_$tb\_V" "sdp RTL_V BASIC_$conf" \
			"tb TESTBENCH_$conf" -io_config IOCFG_$conf -verilog_top_modules glbl -argv $ARGV
		    
		} else {
		    define_sim_config "BASIC_$conf\_$tb\_V" "sdp RTL_V BASIC_$conf" \
			"tb TESTBENCH_$conf" -io_config IOCFG_$conf -argv $ARGV

		    # TODO: not working. Joules can't parse the PLMs Verilog, it throws errors.
		    # define_power_config "P_BASIC_$conf\_$tb" "BASIC_$conf\_$tb\_V" -module sdp \
			#   -command bdw_runjoules
		}
	    }; # foreach tb

	}; # foreach dma

    # }; # foreach pts

}; # foreach chk
