
if [ "$(cat /tmp/N_TCB 2>/dev/null || echo 0)" -eq 0 ]; then
    Log_Message_Print ERROR DAQ_Run_Start "TCB Board is not connected."
    return 1
fi

if [ "$(cat /tmp/N_FADC500 2>/dev/null || echo 0)" -eq 0 ]; then
    Log_Message_Print ERROR DAQ_Run_Start "FADC500 is not connected."
    return 1
fi

if [ "$(cat /tmp/is_DAQ_Run_Started 2>/dev/null || echo 0)" -eq 1 ]; then
    Log_Message_Print ERROR DAQ_Run_Start "DAQ already running."
    return 1
fi

print_usage() {
    echo ""
    echo "Usage: . DAQ_Run_Start.sh -hv [HV config] -daq [DAQ config] -mode [mode] -value [value] -split [time] -comment [comment]"
    echo ""
    echo "options:"
    echo "  -hv       HV config file path (optional)"
    echo "  -daq      DAQ config file path (required)"
    echo "  -mode     Mode: 'event' or 'time' (optional)"
    echo "  -value    Number of events (event mode) or seconds (time mode) (required if -m is set)"
    echo "  -split    Time of splitting (sec) the file while running."
    echo "  -comment  Add Run comment (optional)"
    echo ""
    echo "Example:"
    echo "  . DAQ_Run_Start.sh -hv hv.table -daq daq.config"
    echo "  . DAQ_Run_Start.sh -hv hv.table -daq daq.config -mode event -value 1000"
    echo "  . DAQ_Run_Start.sh -hv hv.table -daq daq.config -mode time -value 100"
    echo "  . DAQ_Run_Start.sh -daq dir_script/DAQ/DAQ_default.config -comment BaselineCheck"
    echo ""
}

HV_Config_File_Path=""
DAQ_Config_File_Path=""
Mode=""
Value=""
SplitTime=""
Comment=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -hv)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "HV configuration file is not defined."
                return 1
            fi
            HV_Config_File_Path="$2"
            
            if [[ ! -f "$HV_Config_File_Path" ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "File '$HV_Config_File_Path' not found."
                return 1
            fi
            
            if [[ ! "$HV_Config_File_Path" =~ \.table$ ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "HV configuration file must have .table extension."
                return 1
            fi
            
            shift 2
            ;;
        -daq)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "DAQ configuration file is not defined."
                return 1
            fi
            DAQ_Config_File_Path="$2"
            
            if [[ ! -f "$DAQ_Config_File_Path" ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "File '$DAQ_Config_File_Path' not found."
                return 1
            fi
            
            if [[ ! "$DAQ_Config_File_Path" =~ \.config$ ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "DAQ configuration file must have .config extension."
                return 1
            fi
            
            shift 2
            ;;
        -mode)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "Mode is not defined."
                return 1
            fi
            Mode="$2"
            
            if [[ "$Mode" != "event" && "$Mode" != "time" ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "Mode must be 'event' or 'time'."
                return 1
            fi
            
            shift 2
            ;;
        -value)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "Value is not defined."
                return 1
            fi
            Value="$2"
            
            if ! [[ "$Value" =~ ^[0-9]+$ ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "Value must be integer."
                return 1
            fi
            shift 2
            ;;
         -split)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "Split Time is not defined."
                return 1
            fi
            SplitTime="$2"
            
            if ! [[ "$SplitTime" =~ ^[0-9]+$ ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "Split Time must be integer."
                return 1
            fi

            shift 2
            ;;
        -comment)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR DAQ_Run_Start "Comment is not defined."
                return 1
            fi
            Comment="$2"
            shift 2
            ;;
        *)
            Log_Message_Print ERROR DAQ_Run_Start "Invalid option: $1"
            return 1
            ;;
    esac
done

if [[ -z "$HV_Config_File_Path" ]] && [[ -z "$DAQ_Config_File_Path" ]]; then
    print_usage
    return 1
fi

if [[ -z "$HV_Config_File_Path" ]]; then
    Log_Message_Print INFO DAQ_Run_Start "HV configuration file (-hv) is not defined."
fi

if [[ -z "$DAQ_Config_File_Path" ]]; then
    Log_Message_Print ERROR DAQ_Run_Start "DAQ configuration file (-daq) is required."
    return 1
fi

if [[ -n "$Mode" && -z "$Value" ]]; then
    Log_Message_Print ERROR DAQ_Run_Start "Value (-value) is required when mode (-mode) is set."
    return 1
fi

if [[ -z "$SplitTime" ]]; then
    Log_Message_Print ERROR DAQ_Run_Start "Split Time (-split) is required."
    return 1
fi

if [ "$(cat /tmp/is_DB_Connected 2>/dev/null || echo 0)" -eq 0 ]; then
    Log_Message_Print WARNING DAQ_Run_Start "Orignal DB ${DAQ_Run_DB_Path} not found. DAQ data will be saved on location /home/cupsoft/nueye_proto/dir_raw."
    export DAQ_Run_DB_Path="dir_raw"
fi


Timestamp=$(date +%Y%m%d%H%M%S)
DAQ_Run_Folder_Path="${DAQ_Run_DB_Path}/${Run_Folder_Name}"
if [[ -n "$Comment" ]]; then
    DAQ_Run_Sub_Folder_Path=${DAQ_Run_Folder_Path}/${Timestamp}__${Comment}
else
    DAQ_Run_Sub_Folder_Path=${DAQ_Run_Folder_Path}/${Timestamp}
fi

mkdir -p ${DAQ_Run_Folder_Path}
mkdir ${DAQ_Run_Sub_Folder_Path}

if [[ -n "$HV_Config_File_Path" ]]; then
    HV_Config_File_Name=$(basename ${HV_Config_File_Path})
    cp ${HV_Config_File_Path} ${DAQ_Run_Sub_Folder_Path}/${HV_Config_File_Name}
fi

DAQ_Config_File_Name=$(basename ${DAQ_Config_File_Path})
cp ${DAQ_Config_File_Path} ${DAQ_Run_Sub_Folder_Path}/${DAQ_Config_File_Name}

DAQ_Run_File_Path=${DAQ_Run_Sub_Folder_Path}/Run.root
echo "$DAQ_Run_File_Path" > /tmp/DAQ_Run_File_Path


Log_Message_Print INFO Run_Start "Starting DAQ run"
Log_Message_Print INFO Run_Start "Run Folder: ${DAQ_Run_Sub_Folder_Path}"

if [[ -n "$HV_Config_File_Path" ]]; then
    Log_Message_Print INFO Run_Start "HV Config: ${HV_Config_File_Path}"
fi

Log_Message_Print INFO Run_Start "DAQ Config: ${DAQ_Config_File_Path}"
Log_Message_Print INFO Run_Start "Comment: ${Comment}"

N_Channel=0
RL_first=""

while IFS= read -r line; do
    if [[ "$line" =~ ^FADCT ]]; then
        fadct_channels=$(echo "$line" | awk '{print $3}')
        in_fadct=1
        enabled=0
        rl=""
    fi

    if [[ $in_fadct -eq 1 ]]; then
        if [[ "$line" =~ ^[[:space:]]*ENABLED ]]; then
            enabled=$(echo "$line" | awk '{print $2}')
        fi
        if [[ "$line" =~ ^[[:space:]]*RL ]]; then
            rl=$(echo "$line" | awk '{print $2}')
        fi
    fi

    if [[ "$line" =~ ^END ]]; then
        if [[ $enabled -ne 0 ]]; then
            N_Channel=$((N_Channel + fadct_channels))
            if [[ -z "$RL_first" ]]; then
                RL_first=$rl
            fi
        fi
        in_fadct=0
    fi
done < $DAQ_Config_File_Path

echo 1 > /tmp/is_DAQ_Run_Started

MAX_ITER=5

if [[ "$Mode" == "event" ]]; then
    Log_Message_Print INFO Run_Start "Mode: Event, Value: ${Value} events"
    for (( i=0; i<MAX_ITER; i++ )); do
        ./PROGRAM/CUPDAQ/build/DAQ/test/tcbdaq -f -c ${DAQ_Config_File_Path} -o ${DAQ_Run_File_Path} -n ${Value} -p ${SplitTime} 2>&1 | tee -a "$Run_Log_File"
        sleep 5
        sudo /usr/local/bin/reset_usb.sh

        Expected_File_Size=$(( Value * N_Channel * ((RL_first * 64) - 16) / 2 ))
        ACTUAL_BYTES=$(du -sb "$DAQ_Run_Sub_Folder_Path" | awk '{print $1}')
        LOWER=$(( Expected_File_Size * 70 / 100 ))

        if [[ "$ACTUAL_BYTES" -ge "$LOWER" ]]; then
            Log_Message_Print INFO DAQ_Run_Start "File size NORMAL: ${ACTUAL_BYTES} B. Proceeding."
            break
        fi

        Log_Message_Print WARNING DAQ_Run_Start "[$((i+1))/${MAX_ITER}] File size abnormal: ${ACTUAL_BYTES} B < ${LOWER} B. Deleting and restarting..."
        find "$DAQ_Run_Sub_Folder_Path" -name "*root*" -type f -delete

        if [[ $((i+1)) -eq $MAX_ITER ]]; then
            Log_Message_Print ERROR DAQ_Run_Start "Max retries reached. DAQ failed. Turn on&off DAQ Hardware."
        fi
    done

elif [[ "$Mode" == "time" ]]; then
    Log_Message_Print INFO Run_Start "Mode: Time, Value: ${Value} seconds"
    ./PROGRAM/CUPDAQ/build/DAQ/test/tcbdaq -f -c ${DAQ_Config_File_Path} -o ${DAQ_Run_File_Path} -t ${Value} -p ${SplitTime} 2>&1 | tee -a "$Run_Log_File"
    Log_Message_Print_Empty
    sleep 5
    sudo /usr/local/bin/reset_usb.sh

else
    Log_Message_Print INFO Run_Start "Mode: Continuous"
    ./PROGRAM/CUPDAQ/build/DAQ/test/tcbdaq -f -c ${DAQ_Config_File_Path} -o ${DAQ_Run_File_Path} -p ${SplitTime} 2>&1 | tee -a "$Run_Log_File"
    Log_Message_Print_Empty
    sleep 5
    sudo /usr/local/bin/reset_usb.sh
fi

echo 0 > /tmp/is_DAQ_Run_Started

