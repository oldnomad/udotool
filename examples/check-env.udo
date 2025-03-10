#!./udotool -i
udotool open
puts "Environment:"
foreach key [array names env] { puts "- $key=$env($key)" }
puts ""
set sys_name [udotool sysname]
puts [string cat "Protocol version: " [udotool protocol]]
puts [string cat "Device dirname:   " $sys_name]
puts [string cat "Device name:      " [exec cat /sys/devices/virtual/input/$sys_name/name]]
puts [string cat "Device modalias:  " [exec cat /sys/devices/virtual/input/$sys_name/modalias]]
