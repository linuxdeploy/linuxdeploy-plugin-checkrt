if [ -x ./checkrt.d/checkrt ]; then
  LD_LIBRARY_PATH="$(./checkrt.d/checkrt):$LD_LIBRARY_PATH"
fi

if [ -e ./checkrt.d/exec.so ]; then
  LD_PRELOAD="./checkrt.d/exec.so:$LD_PRELOAD"
fi
