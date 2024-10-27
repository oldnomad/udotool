#!./udotool -i
set a 0
loop 5
  set a $(($a + 1))
  set b 0
  echo TopIter: "$a" "/$UDOTOOL_LOOP_COUNT/"
  loop 3
      set b $[$b + 1]
      echo Iteration: "$a" "$b" "/$UDOTOOL_LOOP_COUNT/"
  end
end
echo Final: "$a" "$b" "/$UDOTOOL_LOOP_COUNT/"
loop 3
  set a $[$a + 1]
  echo PostIter: "$a" "/$UDOTOOL_LOOP_COUNT/"
end
set rand $(dd if=/dev/urandom bs=4 count=1 status=none | od -t u4 -A n)
echo Random number: $rand
if $((($rand / 2) * 2)) -eq $rand
  loop 2
    echo $rand is even!
  end
else
  loop 3
    echo $rand is odd!
  end
end
