#!/bin/sh -e

appdir=$(dirname "$0")

cxxpre=""
gccpre=""
execpre=""
libc6arch="libc6,x86-64"

desktopfile=$(ls -1 "$appdir"/*.desktop | head -n1)

if [ ! -f "$desktopfile" ]; then
    echo "No desktop file found!"
    exit 1
fi

binary="$appdir"/usr/bin/$(sed -n 's|^Exec=||p' "$desktopfile")

if [ -n "$APPIMAGE" ] && [ "$(file -b "$APPIMAGE" | cut -d, -f2)" != " x86-64" ]; then
  libc6arch="libc6"
fi

if [ -e "$appdir"/usr/optional/libstdc++/libstdc++.so.6 ]; then
  lib=$(PATH="/sbin:$PATH" ldconfig -p | grep "libstdc++\.so\.6 ($libc6arch)" | awk 'NR==1{print $NF}')
  sym_sys=$(tr '\0' '\n' < "$lib" | grep -e '^GLIBCXX_3\.4' | tail -n1)
  sym_app=$(tr '\0' '\n' < "$appdir"/usr/optional/libstdc++/libstdc++.so.6 | grep -e '^GLIBCXX_3\.4' | tail -n1)
  if [ $(printf "${sym_sys}\n${sym_app}"| sort -V | tail -1) != "$sym_sys" ]; then
    cxxpath="$appdir"/optional/libstdc++:
  fi
fi

if [ -e "$appdir"/usr/optional/libgcc/libgcc_s.so.1 ]; then
  lib="$(PATH="/sbin:$PATH" ldconfig -p | grep "libgcc_s\.so\.1 ($libc6arch)" | awk 'NR==1{print $NF}')"
  sym_sys=$(tr '\0' '\n' < "$lib" | grep -e '^GCC_[0-9]\\.[0-9]' | tail -n1)
  sym_app=$(tr '\0' '\n' < "$appdir"/usr/optional/libgcc/libgcc_s.so.1 | grep -e '^GCC_[0-9]\\.[0-9]' | tail -n1)
  if [ "$(printf "${sym_sys}\n${sym_app}"| sort -V | tail -1)" != "$sym_sys" ]; then
    gccpath="$appdir"/optional/libgcc:
  fi
fi

if [ -n "$cxxpath" ] || [ -n "$gccpath" ]; then
  if [ -e "$appdir"/usr/optional/exec.so ]; then
    execpre=""
    export LD_PRELOAD="$appdir"/optional/exec.so:"$LD_PRELOAD"
  fi
  export LD_LIBRARY_PATH="${cxxpath}${gccpath}${LD_LIBRARY_PATH}"
fi

#echo ">>>>> $LD_LIBRARY_PATH"
#echo ">>>>> $LD_PRELOAD"

exec "$binary" "$@"

