#!/bin/sh
echo "Mounting cgroup"
mount -t tmpfs none /sys/fs/cgroup
echo "Creating directory for cgroup"
mkdir /sys/fs/cgroup/memory
echo "Mounting memory cgroup"
mount -t cgroup -o memory memory /sys/fs/cgroup/memory
echo "Creating directory for memory cgroup"
mkdir /sys/fs/cgroup/memory/mem
echo "Setting memory limit"
echo $$ > /sys/fs/cgroup/memory/mem/tasks
echo 20M > /sys/fs/cgroup/memory/mem/memory.limit_in_bytes
echo "Running program"
./ex2
