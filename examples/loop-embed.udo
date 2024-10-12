#!./udotool -i
set a 0
loop 5
  set a $(($a + 1))
  set b 0
  loop 3
      set b $[$b + 1]
      echo Iteration: "$a" "$b"
  end
end
echo Final: "$a" "$b"
loop 3
  set a $[$a + 1]
  echo PostIter: "$a"
end
