

taskset -c 1 ./process_1 &
taskset -c 1 ./process_2

pid=$(ps -ef | grep "process_1" | grep -v grep | awk '{print $2}')
echo ${pid}
# kill ${pid}
