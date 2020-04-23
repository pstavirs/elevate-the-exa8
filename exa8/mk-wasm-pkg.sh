#!/bin/bash

# create ostinato-wasm.tgz containing all required files for the target
cd ../client
cp icons/logo.png .
sed -i 's/qtlogo.svg.*style/logo.png\" style/' ostinato.html
tar cvfz ../exa8/ostinato-wasm.tgz ostinato.html ostinato.js ostinato.wasm qtloader.js logo.png
cd -
