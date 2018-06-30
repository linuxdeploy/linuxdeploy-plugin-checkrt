#! /bin/bash

set -x
set -e

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

cd "$APPDIR"

dd if="$script" skip="$OFFSET" iflag=skip_bytes,count_bytes | tar xvz

# important: exit before the appended tarball
exit
