#!/bin/bash
# DeleteCuts.sh v0.9 (part of MovieCutter)
# Removes orphaned .cut files that have been created by MovieCutter
# (C) 2015 Christian Wünsch

cd /mnt/hd/DataFiles

# Simple version: just look in /DataFiles
# (from http://unix.stackexchange.com/questions/214477)
for f in *.cut *.cut.bak
do
    f="${f%%.bak}"
    f="${f%%.cut}"
    [ "$f" = '*' ] || [ -f "$f.mpg.inf" ] || [ -f "$f.rec.inf" ] || echo rm -f -- "$f.cut" "$f.cut.bak"
done

# Extended version: also look into sudirectories
# Needs 'find' from busybox (installed with TMSTelnet TAP)!
# (from http://stackoverflow.com/questions/4638874)
if [ "$1" = '--recursive' ] ; then
    IFS=$'\n'
    for f in $(find . -type f \( -name '*.cut' -o -name '*.cut.bak' \))
    do
        f="${f%%.bak}"
        f="${f%%.cut}"
        [ "$f" = '*' ] || [ -f "$f.mpg.inf" ] || [ -f "$f.rec.inf" ] || echo rm -f -- "$f.cut" "$f.cut.bak"
    done
fi

# Extended version: slower and problem with quotes in filenames
# (from http://unix.stackexchange.com/questions/212696)
#if [ "$1" = '--recursive' ] ; then
#    find . -type f \( -name '*.cut' -o -name '*.cut.bak' \) -exec bash -c '[ -f "$(echo "{}" | sed -r "s/(.*).cut(.bak)?$/\1/").rec.inf" -o -f "$(echo "{}" | sed -r "s/(.*).cut(.bak)?$/\1/").mpg.inf" ] || echo rm -f -- "{}"' \;
#fi