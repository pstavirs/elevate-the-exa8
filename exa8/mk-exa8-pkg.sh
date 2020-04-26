#!/bin/sh

#
# Add the drone agent files to exa8-ostinato.tgz
# (we assume exa8-ostinato.tgz already contains the webapp binaries)
#
# Needs GNU tar
#
gunzip exa8-ostinato.tgz
tar rvf ../exa8/exa8-ostinato.tar --transform='s,^,exa8-ostinato/,' \
    drone.ini \
    -C ../server drone
gzip exa8-ostinato.tar
mv exa8-ostinato.tar.gz exa8-ostinato.tgz
