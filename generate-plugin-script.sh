#! /bin/bash

set -x
set -e

template=linuxdeploy-plugin-checkrt.template.sh
tarball=checkrt.tar.gz
out=linuxdeploy-plugin-checkrt.sh

make tarball -j$(nproc)

template_size=$(du -b "$template" | awk '{print $1}')
offset=$(($template_size + 2))

sed "s|OFFSET=-1|OFFSET=$offset|g" "$template" > "$out"
dd if="$tarball" bs=1024 >> "$out"
chmod +x "$out"
