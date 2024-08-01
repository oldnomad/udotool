#!./udotool -i
sleep 5
# First we set position to the center of the screen
position 50 50
# Now we loop 10 times, but for no longer than 10.1 seconds
loop -time 10.1 10
  # Move to south-east and double-click
  move 10 10
  key -repeat 2 BTN_LEFT
  sleep 0.5
  # Move to north-west and double-click
  move -10 -10
  key -repeat 2 BTN_LEFT
  sleep 0.5
endloop
