#! /bin/dash
while true 
do 
  changed=0
  git remote update && git status -uno | grep -q 'Your branch is behind' && changed=1
  if [ $changed = 1 ]; then
    ./updater
  fi
    ./bot
done     