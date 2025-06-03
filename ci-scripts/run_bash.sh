#!/bin/bash

remote_server="10.1.1.61"
remote_log_path="~/nearby.log"
local_dir="./nearby1" 


for i in {1..30}
do
python3 run_sl_test.py --test usrp --net 0 --save ~/result_1.txt
scp hechenyi@$remote_server:$remote_log_path $local_dir/nearby$i.log

done 


