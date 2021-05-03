#!/bin/bash

git pull
commit=$(git rev-parse --short HEAD)
echo Commit: $commit
cmake --build build --config Release

mkdir -p sbatch/out
mkdir -p sbatch/tmp
rm -f sbatch/tmp/*

#declare -a arr=(1 2)
#for n in "${arr[@]}"
#for n in 2
#for n in $(seq 1 2)
for i in 1,14 1,28 2,15 2,28 3,19 3,28 4,22 4,28
do
  clusters=cm2_tiny
  partition=cm2_tiny
# nodes=$n
# tasks=28
  nodes=${i%,*}
  tasks=${i#*,}
  sed "
  s/\$commit/$commit/g
  s/\$clusters/$clusters/g
  s/\$partition/$partition/g
  s/\$nodes/$nodes/g
  s/\$ntasks-per-node/$tasks/g
  " sbatch/template.sbatch > sbatch/tmp/$commit-$partition-$nodes-$tasks.sbatch
  sbatch sbatch/tmp/$commit-$partition-$nodes-$tasks.sbatch
done
