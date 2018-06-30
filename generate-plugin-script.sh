#! /bin/bash

template=linuxdeploy-plugin-checkrt.template.sh
tarball=checkrt.tar.gz
out=linuxdeploy-plugin-checkrt.sh

if [ ! -f "$tarball" ]; then
    make tarball -j$(nproc)
fi

offset=$(($(du -b "$template" | awk '{print $1}') + 1))

sed "s|OFFSET=-1|OFFSET=$offset|g" "$template" > "$out"
cat "$tarball" >> "$out"
chmod +x "$out"
