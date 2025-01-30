#!./udotool -i
open
puts "Environment:"
foreach key [array names env] { puts "- $key=$env($key)" }
puts ""
puts [string cat "Device dirname:  " ${::udotool::sys_name}]
puts [string cat "Device name:     " [exec cat /sys/devices/virtual/input/${::udotool::sys_name}/name]]
puts [string cat "Device modalias: " [exec cat /sys/devices/virtual/input/${::udotool::sys_name}/modalias]]
