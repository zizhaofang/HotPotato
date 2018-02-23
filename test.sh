#! /bin/bash                                                                                                                  
pkill -9 ringmaster
pkill -9 player

make clean
git pull
make

RINGMASTER_HOSTNAME=vcm-2932.vm.duke.edu
RINGMASTER_PORT=9876
NUM_PLAYERS=10
NUM_HOPS=300

./ringmaster $RINGMASTER_PORT $NUM_PLAYERS $NUM_HOPS &

sleep 2

for (( i=0; i<$NUM_PLAYERS; i++ ))
do
    ./player $RINGMASTER_HOSTNAME $RINGMASTER_PORT &
done

wait