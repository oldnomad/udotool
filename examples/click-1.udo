#!./udotool -i
echo "Click will start in 5 seconds"
sleep 5
# Signal start
exec /usr/bin/aplay /usr/share/sounds/sound-icons/start
# Click for 33 seconds with delay 25ms
key -delay 0.025 -time 33 BTN_LEFT
# Signal finish
exec /usr/bin/aplay /usr/share/sounds/sound-icons/finish
