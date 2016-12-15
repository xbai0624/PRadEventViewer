#!/bin/bash
# usage: event_sel.sh <begin_run> <end_run>
# auto search the replayed dst files and event selection list to cut off bad events

# define program position
prog="/work/hallb/prad/PRadEventViewer/decoder/eventSelect"
# define replay files directory
dir="/lustre/expphy/work/hallb/prad/replay/"
# define event list
bad_base="/lustre/expphy/work/hallb/prad/replay_EnS/EventSelect_[run].txt"

# check run number range
# begin
if [[ $1 -eq 0 ]]; then
    run_begin=0
else
    run_begin=$1
fi

# end
if [[ $2 -eq 0 ]]; then
    run_end=999999
else
    run_end=$2
fi

# execute program
for file in $dir*.dst; do
    run=`echo $file | egrep -o "[0-9]+"`
    bad_file=${bad_base//\[run\]/$run}
    if [ "$run" -ge "$run_begin" ] && [ "$run" -le "$run_end" ]; then
        echo $prog $file $bad_file
        $prog $file $bad_file
    fi
done
