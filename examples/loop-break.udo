#!./udotool -i
echo Test 1: break immediate
loop 3
  loop 5
    if "$UDOTOOL_LOOP_COUNT" -eq 3
       echo Breaking...
       break
    end
    echo Remaining: "$UDOTOOL_LOOP_COUNT"
  end
  echo Top loop remaining: "$UDOTOOL_LOOP_COUNT"
end
echo End of top loop
echo
echo Test 2: break twice
loop 3
  loop 5
    if "$UDOTOOL_LOOP_COUNT" -eq 3
       echo Breaking...
       break 2
    end
    echo Remaining: "$UDOTOOL_LOOP_COUNT"
  end
  echo Top loop remaining: "$UDOTOOL_LOOP_COUNT"
end
echo End of top loop
