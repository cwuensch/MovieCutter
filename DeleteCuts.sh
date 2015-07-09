#!/bin/bash
# DeleteCuts.sh v1.0 (part of MovieCutter)
# Removes orphaned .cut files that have been created by MovieCutter
# (C) 2015 Christian Wünsch

function DeleteCuts() {
    # Simple version: just look in the current folder
    # (from http://unix.stackexchange.com/questions/214477)
    for f in *.cut *.cut.bak
    do
        f="${f%%.bak}"
        f="${f%%.cut}"
        [ "$f" = '*' ] || [ -f "$f.rec.inf" ] || [ -f "$f.mpg.inf" ] || rm -f -- "$f.cut" "$f.cut.bak"
    done

    # Extended version: also look into subdirectories
    # (from http://stackoverflow.com/questions/4638874)
    if [ "$1" = '--recursive' ] ; then
        for d in *
        do
            if [ -d "$d" ] ; then
                (cd -- "$d" ; DeleteCuts $1)
            fi
        done
    fi
}

cd /mnt/hd/DataFiles
DeleteCuts $1