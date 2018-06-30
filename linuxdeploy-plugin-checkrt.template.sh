#! /bin/bash

OFFSET=-1

if [ $OFFSET -le 0 ]; then
    echo "Please run ./generate-plugin-script.sh to build the linuxdeploy plugin script"
    exit 1
fi

script=$(readlink -f "$0")

show_usage() {
    echo "Usage: $script --appdir <path to AppDir>"
    echo
    echo "Creates or replaces AppRun in AppRun that decides whether to load a bundled libstdc++ or not"
}

APPDIR=

case "$1" in
    --plugin-api-version)
        echo "0"
        exit 0
        ;;
    --appdir)
        APPDIR="$2"
        shift
        shift
        ;;
    *)
        echo "Invalid argument: $1"
        echo
        show_usage
        exit 1
esac

if [ ! -d "$APPDIR" ]; then
    echo "No such directory: $APPDIR"
    exit 1
fi

pushd "$APPDIR" &>/dev/null

# extract files from appended tarball
echo "Extracting binaries"
dd if="$script" skip="$OFFSET" iflag=skip_bytes,count_bytes 2>/dev/null | tar -xz

# copy system libraries
mkdir -p usr/optional/{libstdc++,libgcc}

for path in /usr/lib/x86_64-linux-gnu/libstdc++.so.6; do
    if [ -f "$path" ]; then
        echo "Copying into AppDir:: $path"
        cp "$path" usr/optional/libstdc++/
    fi
done

for path in /lib/x86_64-linux-gnu/libgcc_s.so.1 /lib/x86_64-linux-gnu/libm.so.6 lib/x86_64-linux-gnu/libc.so.6; do
    if [ -f "$path" ]; then
        echo "Copying into AppDir: $path"
        cp "$path" usr/optional/libgcc/
    fi
done

if [ -f AppRun ]; then
    rm AppRun
fi

# use patched AppRun
mv AppRun.sh AppRun

# leave AppDir
popd &>/dev/null

# important: exit before the appended tarball
exit
