#!./udotool -i
set a 0
loop 10
  set a $[$a + 1]
  echo Iteration: "$a"
  if $(($a - 3))
    echo Not yet
  else
    exit
  end
end
