#!./udotool -i
set a 0
loop 10
  set a $[$a + 1]
  echo Iteration: "$a"
  if "$a" -ge 3
    exit
  else
    echo "Not yet"
  end
end
