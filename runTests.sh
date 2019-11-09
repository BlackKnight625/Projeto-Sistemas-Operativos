#!/bin/sh
inputdir=$1
outputdir=$2
maxthreads=$3
numbuckets=$4
for f in $(ls $inputdir)
do
    inputfile=$inputdir/$f
    outputfile=$outputdir/$f-1.txt
    touch $outputfile
    echo Inputfile=$inputfile Numthreads=1
    ./tecnicofs-nosync $inputfile $outputfile 1 1 | grep TecnicoFs
done
for f in $(ls $inputdir)
do
    for numthreads in $(seq 2 $maxthreads)
    do
        inputfile=$inputdir/$f
        outputfile=$outputdir/$f-$numthreads.txt
        touch $outputfile
        echo Inputfile=$inputfile Numthreads=$numthreads
        ./tecnicofs-mutex $inputfile $outputfile $numthreads $numbuckets | grep TecnicoFs
    done
done