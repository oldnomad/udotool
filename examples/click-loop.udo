#!./udotool -i
open
sleep 5
# First we set position to the center of the screen
position 50 50
# Now we loop 10 times, but for no longer than 10.1 seconds
timedloop 10.1 10 t n {
  puts "Count: $n, time: $t"
  # Move to south-east and double-click
  move 24 24
  key -repeat 2 BTN_LEFT
  if { [rand 10] == 5 } { puts "Random break!"; break }
  sleep 0.5
  # Move to north-west and double-click
  move -24 -24
  key -repeat 2 BTN_LEFT
  sleep 0.5
}
