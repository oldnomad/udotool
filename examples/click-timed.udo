#!./udotool -i
set DELAY 5
puts "Click will start in $DELAY seconds"
sleep $DELAY
# Signal start
exec /usr/bin/aplay /usr/share/sounds/sound-icons/start
# Click for 33 seconds with delay 25ms
key -delay 0.025 -time 33 BTN_LEFT
# Signal finish
exec /usr/bin/aplay /usr/share/sounds/sound-icons/finish
