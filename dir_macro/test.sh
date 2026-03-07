
. dir_script/HV_Create_Config.sh -hv dir_config/HV/HV_default.table -output dir_config/HV/test0.table -value s s 1240 s s 100 -apply

. dir_script/HV_Log_Start.sh

. dir_script/HV_Control.sh -hv dir_config/HV/HV_default.table -command on

sleep 10


val=(1200 s 1200 s 1200)

. dir_script/ROOT/test.sh Hello

. dir_script/HV_Create_Config.sh -hv dir_config/HV/HV_default.table -output dir_config/HV/test0.table -value "${val[@]}"

. dir_script/DAQ_Run_Start.sh -daq dir_config/DAQ/DAQ_default.config -mode time -value 600 -split 60 -comment alpha
. dir_script/DAQ_Run_Start.sh -daq dir_config/DAQ/DAQ_default.config -mode time -value 600 -split 60 -comment beta
. dir_script/DAQ_Run_Start.sh -daq dir_config/DAQ/DAQ_default.config -mode time -value 600 -split 60 -comment gamma

. dir_script/Script_Scheduler.sh "echo Schedule1 " 10
. dir_script/Script_Scheduler.sh "echo Schedule2 " 20
. dir_script/Script_Scheduler.sh "echo Schedule3 " 30

sleep 10

fallocate -l 2G filename

sleep 60

