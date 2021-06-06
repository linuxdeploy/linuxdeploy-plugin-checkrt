#! /bin/bash

set -x
set -e

template=linuxdeploy-plugin-checkrt.template.sh
tarball=checkrt.tar.gz

if [ "$ARCH" != "" ]; then
    out=linuxdeploy-plugin-checkrt-"$ARCH".sh
else
    out=linuxdeploy-plugin-checkrt.sh
fi

make tarball -j$(nproc)

template_size=$(du -b "$template" | awk '{print $1}')
offset=$(($template_size + 2))

sed "s|OFFSET=-1|OFFSET=$offset|g" "$template" > "$out"
dd if="$tarball" bs=1024 >> "$out"
chmod +x "$out"
