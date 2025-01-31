#!./udotool -i
puts "Axes:"
foreach n [names axis] {
    lassign $n name code
    puts "- $name ($code)"
}
puts "Keys:"
foreach n [names key] {
    lassign $n name code
    puts "- $name ($code)"
}
