#!/bin/sh

# make sure we are root
if [ "`id -u`" -ne "0" ]
then
    echo "Error: You need to run this script using sudo or as root"
    exit 1
fi

echo Bringing up all the interfaces...
ifconfig enp5s0f1 up
ifconfig enp5s0f2 up
ifconfig enp5s0f3 up
ifconfig lan1 up
ifconfig lan2 up
ifconfig lan3 up
ifconfig lan4 up
ifconfig lan5 up
ifconfig lan6 up
ifconfig lan7 up
ifconfig lan8 up

echo Killing any old processes...
pkill -u 0 drone
pkill -f "websockify.*7877.*7878"
pkill -f "python3.*http.server.*8086"

sleep 5

echo Starting websocket proxy...
websockify :7877 :7878 &> /dev/null &

echo Starting Ostinato drone agent...
./drone &

echo Starting HTTP server for the webapp...
cd webapp && python3 -m http.server 8086 &> /dev/null &
