#!./udotool -i
open
echo Environment:
exec env
echo ""
echo Device name: "$(cat /sys/devices/virtual/input/$UDOTOOL_SYSNAME/name)"
echo Device modalias: "$(cat /sys/devices/virtual/input/$UDOTOOL_SYSNAME/modalias)"
