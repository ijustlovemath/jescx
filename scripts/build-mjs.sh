#!/bin/bash
#source $HOME/.nvm/nvm.sh
if [ ! -d "./src" ]; then
    echo "call this from root of repo"
    exit 1
fi
npx esbuild --bundle src/foo.js --format=esm --outfile=src/my-foo-lib.mjs
