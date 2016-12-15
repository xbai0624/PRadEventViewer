#!/bin/bash
# usage: cosmic_rej.sh <begin_run> <end_run>
# auto search the replayed dst files and reject cosmic backgrounds by using a specified program

# define program position
prog="/work/hallb/prad/PRadEventViewer/decoder/cosmicCheck"
# define replay files directory
dir="/lustre/expphy/work/hallb/prad/replay/"

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
    if [ "$run" -ge "$run_begin" ] && [ "$run" -le "$run_end" ]; then
        echo $prog $file
        $prog $file
    fi
done
