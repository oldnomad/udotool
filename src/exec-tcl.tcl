# SPDX-License-Identifier: GPL-3.0-or-later
#
# Tcl macros
#
set ::udotool::default_delay 0.05

proc ::internal::sysfile {name} {{extlist {glob.tcl nshelper.tcl oo.tcl stdlib.tcl tclcompat.tcl tree.tcl}}} {
    if {$name == [info script]} {return 1}
    return [lsearch -bool $extlist $name]
}

proc ::internal::getopt {_argv opt {hasval 0} {defval "--"}} {
    upvar $_argv argv
    set idx [lsearch $argv $opt]
    if { $idx == -1 } { return $defval }
    set argv [lreplace $argv $idx $idx]
    if { $hasval == 0 } { return 1 }
    if { [llength $argv] <= $idx } {
        if { $hasval == 2 } { return "" }
        error [format "option \"%s\" requires a value" opt] [info stacktrace]
    }
    set val  [lindex $argv $idx]
    set argv [lreplace $argv $idx $idx]
    return $val
}

proc input {args} {
    udotool input {*}[lmap arg $args { split $arg {=} }]
}

proc keydown {args} {
    eval udotool input {*}[lmap key $args { list [list KEYDOWN $key] SYNC }]
}

proc keyup {args} {
    eval udotool input {*}[lmap key [lreverse $args] { list [list KEYUP $key] SYNC }]
}

proc wheel {args} {
    set axis REL_WHEEL
    if { [::internal::getopt args -h] != "--" } { set axis REL_HWHEEL }
    if { [llength $args] != 1 } {
        error "wrong # of arguments: should be \"wheel ?-h? delta\"" [info stacktrace]
    }
    udotool input [list $axis $args]
}

proc move {args} {
    set prefix REL_
    if { [::internal::getopt args -r] != "--" } { set prefix REL_R }
    set argn [llength $args]
    if { $argn < 1 || $argn > 3 } {
        error "wrong # of arguments: should be \"move ?-r? delta_x ?delta_y? ?delta_z?\"" [info stacktrace]
    }
    udotool input {*}[lmap val $args axis {X Y Z} {
        if { "$val" == "" } { break }
        list "$prefix$axis" $val
    }]
}

proc position {args} {
    set prefix ABS_
    if { [::internal::getopt args -r] != "--" } { set prefix ABS_R }
    set argn [llength $args]
    if { $argn < 1 || $argn > 3 } {
        error "wrong # of arguments: should be \"position ?-r? pos_x ?pos_y? ?pos_z?\"" [info stacktrace]
    }
    udotool input {*}[lmap val $args axis {X Y Z} {
        if { "$val" == "" } { break }
        list "$prefix$axis" $val
    }]
}

proc key {args} {
    set rep_num   [::internal::getopt args -repeat 1 0]
    set rep_time  [::internal::getopt args -time   1 0]
    set rep_delay [::internal::getopt args -delay  1 $::udotool::default_delay ]
    set down_list [lmap key $args { list KEYDOWN $key }]
    set up_list   [lmap key [lreverse $args] { list KEYUP $key }]
    set key_list  [list {*}$down_list SYNC {*}$up_list]
    if { $rep_num == 0 } {
        set rep_num [expr { $rep_time <= 0 ? 1 : -1 }]
    }
    udotool open
    timedloop $rep_time $rep_num {
        udotool input {*}$key_list
        sleep $rep_delay
    }
}

if { $::udotool::debug > 0 } {
    xtrace [lambda {mode fname line result cmd arglist} {
        if { $mode != "cmd" } { return }
        set level 1
        if { [::internal::sysfile $fname] } {
            if { $::udotool::debug < 3 } { return }
            set level 3
        }
        puts "\[$level\] \[$fname:$line\] $cmd $arglist"
    }]
}
