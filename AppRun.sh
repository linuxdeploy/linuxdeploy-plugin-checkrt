#!/bin/sh -e

appdir=$(readlink -f ${APPDIR:-$(dirname "$0")})

desktopfile=$(ls -1 "$appdir"/*.desktop | head -n1)

if [ ! -f "$desktopfile" ]; then
    echo "No desktop file found!"
    exit 1
fi

library_path=""
library_type=

binary="$appdir"/usr/bin/$(sed -n 's|^Exec=||p' "$desktopfile" | cut -d' ' -f1)

for lib_tuple in libstdc++.so.6:'^GLIBCXX_[0-9]\.[0-9]' libgcc_s.so.1:'^GCC_[0-9]\.[0-9]'; do
    lib_filename=$(echo "$lib_tuple" | cut -d: -f1)
    version_prefix=$(echo "$lib_tuple" | cut -d: -f2)
    lib_dir="$appdir"/usr/optional/"$lib_filename"/
    lib_path="$lib_dir"/"$lib_filename"
    if [ -e "$lib_path" ]; then
      lib=$(PATH="/sbin:$PATH" ldconfig -p | grep "$lib_filename" | awk 'NR==1 {print $NF}')
      sym_sys=$(tr '\0' '\n' < "$lib" | grep -e "$version_prefix" | tail -n1)
      sym_app=$(tr '\0' '\n' < "$lib_path" | grep -e "$version_prefix" | tail -n1)
      if [ z$(printf "${sym_sys}\n${sym_app}" | sort -V | tail -1) != z"$sym_sys" ]; then
        export library_path="$lib_dir"/:"$library_path"
      fi
    fi
done

export LD_LIBRARY_PATH="$library_path:$LD_LIBRARY_PATH"

if [ -n "$cxxpath" ] || [ -n "$gccpath" ]; then
  if [ -e "$appdir"/usr/optional/exec.so ]; then
    export LD_PRELOAD="$appdir"/usr/optional/exec.so:"$LD_PRELOAD"
  fi
fi

#echo ">>>>> $LD_LIBRARY_PATH"
#echo ">>>>> $LD_PRELOAD"

exec "$binary" "$@"

