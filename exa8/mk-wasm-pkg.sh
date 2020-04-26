#!/bin/sh

#
# Create exa8-ostinato.tgz containing all wasm files for the target; the
# companion script mk-exa8-pkg.sh will add the drone agent side files to it
#
# Needs GNU tar
#
cd ../client
cp icons/logo.png .
sed -i 's/qtlogo.svg.*style/logo.png\" style/' ostinato.html
ln -s ostinato.html index.html
tar cvfz ../exa8/exa8-ostinato.tgz --transform='s,^,exa8-ostinato/webapp/,' \
    index.html \
    ostinato.html \
    ostinato.js \
    ostinato.wasm \
    qtloader.js \
    logo.png
rm logo.png
rm index.html
cd -
