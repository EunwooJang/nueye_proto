
(
MY_PID=$BASHPID
loop_count=0
is_CPU_Error=0
is_RAM_Error=0
is_Mem_Error=0

while [ "$(cat /tmp/is_Tracker_Enabled 2>/dev/null || echo 0)" -eq 1 ]; do
    
    Log_Message_Write_Empty
    # CPU Usage
    cpu1=($(head -1 /proc/stat))
    idle1=${cpu1[4]}
    total1=0
    for val in "${cpu1[@]:1}"; do total1=$((total1 + val)); done

    sleep 5

    cpu2=($(head -1 /proc/stat))
    idle2=${cpu2[4]}
    total2=0
    for val in "${cpu2[@]:1}"; do total2=$((total2 + val)); done

    diff_idle=$((idle2 - idle1))
    diff_total=$((total2 - total1))
    CPU_Usage=$(( (diff_total - diff_idle) * 100 / diff_total ))

    # CPU Temperature
    CPU_Temp=$(cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null | awk '{printf "%.1f", $1/1000}')

    # RAM
    RAM_Usage=$(free | grep Mem | awk '{printf "%.0f", $3/$2*100}')

    # DAQ RUN DB
    DAQ_DB_Free_GB=$(df -BG | grep "$Memory_Disk_Name" | awk '{gsub("G","",$4); print $4}')

    # HV Board, TCB FADC500 DB Connection
    if (( loop_count % 6 == 0 )); then

        # HV Board LAN Connection
        if ping -c 1 -W 2 ${HV_Board_IP} > /dev/null 2>&1; then
            echo 1 > /tmp/is_HV_Board_Connected
            is_HV_Board_Connected=1
        else
            echo 0 > /tmp/is_HV_Board_Connected
            is_HV_Board_Connected=0
        fi

        # USB Devices
        USB_OUTPUT=$(lsusb 2>/dev/null)
        N_TCB=$(echo "$USB_OUTPUT" | grep -i "TCB" | wc -l)
        N_FADC500=$(echo "$USB_OUTPUT" | grep -i "FADC500" | wc -l)
        echo $N_TCB > /tmp/N_TCB
        echo $N_FADC500 > /tmp/N_FADC500
        # DB Connection
        if [ -d "${DAQ_Run_DB_Path}" ]; then
            echo 1 > /tmp/is_DB_Connected
            is_DB_Connected=1
        else
            echo 0 > /tmp/is_DB_Connected
            is_DB_Connected=0
        fi
    fi

    
    Log_Message_Write STAT Tracker "CPU: ${CPU_Usage}% | Temp: ${CPU_Temp}°C | RAM: ${RAM_Usage}% | DB: ${is_DB_Connected} | Disk Free: ${DAQ_DB_Free_GB}GB"
    Log_Message_Write STAT Tracker "HV: ${is_HV_Board_Connected}   | TCB: ${N_TCB}    | FADC500: ${N_FADC500}"

    if [[ "$CPU_Usage" -ge "$CPU_Usage_Error_Percent" ]]; then
        Log_Message_Write ERROR Tracker "CPU Usage: ${CPU_Usage}% (>= ${CPU_Usage_Error_Percent}%)"
        echo 1 > /tmp/is_CPU_Too_Busy
        is_CPU_Error=1
    elif [[ "$CPU_Usage" -ge "$CPU_Usage_Warning_Percent" ]]; then
        Log_Message_Write WARNING Tracker "CPU Usage: ${CPU_Usage}% (>= ${CPU_Usage_Warning_Percent}%)"
    fi

    CPU_Temp_Int=$(printf "%.0f" "$CPU_Temp")
    if [[ "$CPU_Temp_Int" -ge "$CPU_Temp_Error_Degree" ]]; then
        Log_Message_Write ERROR Tracker "CPU Temp: ${CPU_Temp}°C (>= ${CPU_Temp_Error_Degree}°C)"
        echo 1 > /tmp/is_CPU_Temp_Too_High
        is_CPU_Error=1
    elif [[ "$CPU_Temp_Int" -ge "$CPU_Temp_Warning_Degree" ]]; then
        Log_Message_Write WARNING Tracker "CPU Temp: ${CPU_Temp}°C (>= ${CPU_Temp_Warning_Degree}°C)"
    fi

    if [[ "$RAM_Usage" -ge "$RAM_Usage_Error_Percent" ]]; then
        Log_Message_Write ERROR Tracker "RAM Usage: ${RAM_Usage}% (>= ${RAM_Usage_Error_Percent}%)"
        echo 1 > /tmp/is_RAM_Too_Busy
        is_RAM_Error=1
    elif [[ "$RAM_Usage" -ge "$RAM_Usage_Warning_Percent" ]]; then
        Log_Message_Write WARNING Tracker "RAM Usage: ${RAM_Usage}% (>= ${RAM_Usage_Warning_Percent}%)"
    fi

    if [[ "$DAQ_DB_Free_GB" -le "$Mem_Error_GB" ]]; then
        Log_Message_Write ERROR Tracker "Disk Free: ${DAQ_DB_Free_GB}GB (<= ${Mem_Error_GB}GB)"
        echo 1 > /tmp/is_Memory_Not_Enough
        is_Mem_Error=1
    elif [[ "$DAQ_DB_Free_GB" -le "$Mem_Warning_GB" ]]; then
        Log_Message_Write WARNING Tracker "Disk Free: ${DAQ_DB_Free_GB}GB (<= ${Mem_Warning_GB}GB)"
    fi

    loop_count=$((loop_count + 1))
    
    # If failure, safety logic 
    if [[ "$is_Mem_Error" -eq 1 ]] || [[ "$is_CPU_Error" -eq 1 ]] || [[ "$is_RAM_Error" -eq 1 ]]; then
        
        Log_Message_Write INFO Tracker "Start Terminating Run for safety"

        echo 1 > /tmp/is_Run_Failure

        # Cancel all scheduled script
        if [[ -f /tmp/Scheduled_Script_PIDs ]]; then
            while read -r spid; do
            if kill -0 "$spid" 2>/dev/null; then
                kill -- -$(ps -o pgid= -p "$spid" | tr -d ' ')
                Log_Message_Write INFO Tracker "Killed Scheduled Script PID:" "$spid"
            fi
            done < /tmp/Scheduled_Script_PIDs

            rm -f /tmp/Scheduled_Script_PIDs
        fi

        # If DAQ run started, End
        if [[ "$(cat /tmp/is_DAQ_Run_Started 2>/dev/null || echo 0)" -eq 1 ]]; then
            touch /tmp/forced.endrun
            sleep 10
            Log_Message_Write INFO Tracker "Terminated DAQ Run for safety."

        fi
        
        # If HV is on, turn off
        if [[ "$(cat /tmp/is_HV_On 2>/dev/null || echo 0)" -eq 1 ]]; then
            (
                flock 200
                HV_File=$(cat /tmp/HV_File 2>/dev/null)
                echo 0 > /tmp/is_HV_On
                echo 1 > /tmp/is_HV_Off_By_Run_Failure
                ./PROGRAM/HVControl/build/hvcontrol -t "$HV_File" -c off
                Log_Message_Write INFO Tracker "Terminated HV for safety."
            ) 200>/tmp/hv_control.lock
        fi

        # Kill currently running scripts (sleep / python3 / root scripts)
        Current_Script_PID=$(cat /tmp/Current_Script_PID 2>/dev/null)
        if [[ -n "$Current_Script_PID" ]]; then
            CHILD_PIDS=$(pgrep -P "$Current_Script_PID" | grep -v "$MY_PID")
            echo "$CHILD_PIDS" | xargs -r kill
            Log_Message_Write INFO Tracker "Killed Current Script PID:" "$Current_Script_PID"
        fi
       
       break
    fi
done

) & disown

Log_Message_Write INFO Tracker "Tracker Started."

sleep 6
