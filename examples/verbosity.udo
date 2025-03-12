#!./udotool -i
puts [string cat "Dry run mode : " [udotool runtime dry_run]]
puts [string cat "Verbosity (1): " [udotool runtime verbose]]
key key_a
udotool runtime verbose 3
puts [string cat "Verbosity (2): " [udotool runtime verbose]]
key key_b
udotool runtime verbose 0
puts [string cat "Verbosity (3): " [udotool runtime verbose]]
key key_c
