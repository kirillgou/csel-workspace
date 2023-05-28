#!/bin/sh
mkdir /sys/fs/cgroup/cpuset
mount -t cgroup -o cpu,cpuset cpuset /sys/fs/cgroup/cpuset
mkdir /sys/fs/cgroup/cpuset/high
mkdir /sys/fs/cgroup/cpuset/low
echo 3 > /sys/fs/cgroup/cpuset/high/cpuset.cpus
echo 0 > /sys/fs/cgroup/cpuset/high/cpuset.mems
echo 2 > /sys/fs/cgroup/cpuset/low/cpuset.cpus
echo 0 > /sys/fs/cgroup/cpuset/low/cpuset.mems


