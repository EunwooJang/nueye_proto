if [ "$(cat /tmp/is_HV_Log_Enabled 2>/dev/null || echo 0)" -eq 1 ]; then
    Log_Message_Print ERROR HV_Log_Start "HV Board is  already started."
    return 1
fi


if [ "$(cat /tmp/is_HV_Board_Connected 2>/dev/null || echo 0)" -eq 0 ]; then
    Log_Message_Print ERROR HV_Log_Start "HV Board is not connected."
    return 1
fi

echo 1 > /tmp/is_HV_Log_Enabled
echo 0 > /tmp/is_HV_Log_Updated



# Start HV Logging as background process
while [ "$(cat /tmp/is_HV_Log_Enabled 2>/dev/null || echo 0)" -eq 1 ]; do

    (
    Log_Message_Print_Empty
    flock 200
    HV_File=$(cat /tmp/HV_File 2>/dev/null)
    ./PROGRAM/HVControl/build/hvcontrol -t "$HV_File" -c print >> $HV_Log_File
    ) 200>/tmp/hvcontrol.lock

    echo 1 > /tmp/is_HV_Log_Updated
    Log_Message_Print INFO HV_Log "HV value logged."
    sleep 5
done &

Log_Message_Print INFO HV_Log "Start HV logging."




# Start HV Log Monitoring as background process
while [ "$(cat /tmp/is_HV_Log_Enabled 2>/dev/null || echo 0)" -eq 1 ]; do
    if [ "$(cat /tmp/is_HV_On 2>/dev/null || echo 0)" -eq 1 ]; then
        sleep 10  # Necessary for preventing False warning. HV needs to rise.

        declare -A reported_channels
        
        while [ "$(cat /tmp/is_HV_On 2>/dev/null || echo 0)"  -eq 1 ]; do

            while [ "$(cat /tmp/is_HV_Log_Updated 2>/dev/null || echo 0)" -eq 0 ]; do
                sleep 1
            done
            
            Log_Message_Print_Empty 
            echo 0 > /tmp/is_HV_Log_Updated
            HV_File=$(cat /tmp/HV_File 2>/dev/null)
            N_HV=$(grep -c '[^[:space:]]' "$HV_File")
            HV_Output=$(tail -n "$N_HV" "$HV_Log_File")

            while IFS= read -r line; do
                read -ra values <<< "$line"
                channel_name="${values[0]}"
                
                v_mon="${values[5]}"
                
                I_mon="${values[6]}"
                I_max="${values[4]}"

                # Condition 1 Current HV is lower than 10 V after HV On -> Shutdown happened
                if (( $(echo "$v_mon < 10.0" | bc -l) )); then
                    if [ -z "${reported_channels[${channel_name}_hv]}" ]; then
                        Log_Message_Print WARNING HV_Monitor "$channel_name HV is dropped to 0V. Check the $HV_Log_File to see detail."
                        reported_channels[${channel_name}_hv]=1
                    fi
                else
                    unset reported_channels[${channel_name}_hv] 
                fi
                
                # Condition 2 Current Current is lower than 10 uA after HV On -> PMT Cable Connection
                if (( $(echo "$I_mon < 10.0" | bc -l) )); then
                    if [ -z "${reported_channels[${channel_name}_low]}" ]; then
                        Log_Message_Print WARNING HV_Monitor "$channel_name Current is too low. Check PMT Connection."
                        reported_channels[${channel_name}_low]=1
                    fi
                else
                    unset reported_channels[${channel_name}_low]
                fi
                
                # Condition 3 Current Current is a bit danger. -> If Current current(I) > V_set/R_set * 1.1, HV killed by hvcontrol. This for warning
                if (( $(echo "$I_mon > $I_max / 1.1 * 1.075" | bc -l) )); then
                    if [ -z "${reported_channels[${channel_name}_high]}" ]; then
                        Log_Message_Print WARNING HV_Monitor "$channel_name Current alomst reached to shutdown threshold."
                        reported_channels[${channel_name}_high]=1
                    fi
                else
                    unset reported_channels[${channel_name}_high]
                fi

            done <<< "$HV_Output"

        done
        unset reported_channels
    fi
    sleep 1
done & disown

Log_Message_Print INFO HV_Monitor "Start HV monitoring."

