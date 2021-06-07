if [ -x "$APPDIR/checkrt.d/checkrt" ]; then
  LD_LIBRARY_PATH="$($APPDIR/checkrt.d/checkrt):$LD_LIBRARY_PATH"
fi

if [ -e "$APPDIR/checkrt.d/exec.so" ]; then
  LD_PRELOAD="$APPDIR/checkrt.d/exec.so:$LD_PRELOAD"
fi
