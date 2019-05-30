if { [info exists env(TECH)] } {
    set TECH $env(TECH)
} else {
    puts "\nERROR: Missing definition of TECH\n"
    exit -1
}

if {$TECH eq "cmos32soi"} {
    # Create a list of operating points(name, clock_period).
    # The name of the operating point must match the name of the folder where
    # the db files are located (e.g. <esp>/tech/cmos32soi/db/1p0v/*.db). This
    # name should also match the suffix used by the standard cell target library
    # (e.g. ibm32soi_hvt_1p0v.db)
    set operating_points [list \
			      [list "1p0v"  1000.0] \
			      [list "0p85v" 1386.0] \
			      [list "0p75v" 1780.0] \
			      [list "0p6v"  2508.0] \
			      ]
    set opp_nominal "1p0v"
    set clk_nominal 1000.0
} else {
    puts "\nERROR: $TECH library is not supported."
}
