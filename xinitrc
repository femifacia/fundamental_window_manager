# 1. Start some programs to play with.
xeyes &
sleep 2 &
#sleep 2 && xterm &

# 2. Start our window manager.
export GLOG_logtostderr=1
exec ./a.out