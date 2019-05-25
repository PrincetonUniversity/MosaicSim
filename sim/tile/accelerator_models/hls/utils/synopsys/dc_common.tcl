suppress_message {UID-401 OPT-200 OPT-1500 UISN-27 OPT-1208 OPT-1215 DDB-72 TIM-164 OPT-1206 OPT-319 TFCHK-014 TFCHK-049 TFCHK-050}
set_host_options -max_cores 4

if { [info exists env(DESIGN_NAME)] } {
    set DESIGN_NAME $env(DESIGN_NAME)
} else {
    puts "\nERROR: Missing definition of DESIGN_NAME\n"
    exit -1
}

if { [info exists env(HLS_ROOT)] } {
    set HLS_ROOT $env(HLS_ROOT)
} else {
    puts "\nERROR: Missing definition of ESP_ROOT\n"
    exit -1
}

if { [info exists env(RTL_PATH)] } {
    set RTL_PATH $env(RTL_PATH)
} else {
    puts "\nERROR: Missing definition of RTL_PATH\n"
    exit -1
}

if { [info exists env(MEMGEN_OUT)] } {
    set MEMGEN_OUT $env(MEMGEN_OUT)
} else {
    puts "\nERROR: Missing definition of MEMGEN_OUT\n"
    exit -1
}

if { [info exists env(TECH)] } {
    set TECH $env(TECH)
} else {
    puts "\nERROR: Missing definition of TECH\n"
    exit -1
}

if {$TECH eq "cmos32soi"} {
    set TARGET_LIBRARY_NAME "ibm32soi_hvt"
    set TARGET_LIBRARY "$TARGET_LIBRARY_NAME\_$opp_nominal"
    set CLOCK_PERIOD $clk_nominal
    set estimated_clock_uncertainty 50
    set estimated_input_delay 100.0
    set estimated_clock_input_delay 220.0
    set estimated_output_delay 100.0
    set default_driving_cell INV_X4M_A9TH
} else {
    puts "\nERROR: $TECH library is not supported."
}

set TOP_NAME    "${DESIGN_NAME}"

set REPORTS_DIR ${DESIGN_NAME}/rep
set RESULTS_DIR ${DESIGN_NAME}/out

file mkdir ${REPORTS_DIR}
file mkdir ${RESULTS_DIR}

set_svf "./${RESULTS_DIR}/${DESIGN_NAME}.svf"

set TECH_PATH $HLS_ROOT/tech/$TECH
