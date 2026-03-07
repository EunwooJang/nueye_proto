
if [ "$(cat /tmp/is_HV_Board_Connected 2>/dev/null || echo 0)" -eq 0 ]; then
    Log_Message_Print ERROR HV_Control "HV Board is not connected."
    return 1
fi


print_usage() {
    echo ""
    echo "Usage: HV_Control.sh -hv [HV configuration file] -command [command]"
    echo ""
    echo "options:"
    echo "  -hv         HV configuration file"
    echo "  -command    Command (mon/set/on/off/print)"
    echo ""
    echo "Example:"
    echo "  ./HV_Control.sh -hv test.table -command on"
    echo ""
}

HV_File=""
Command=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -hv)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR HV_Control "HV configuration file is not defined."
                return 1
            fi
            HV_File="$2"
            
            if [[ ! -f "$HV_File" ]]; then
                Log_Message_Print ERROR HV_Control "File '$HV_File' not found."
                return 1
            fi
            if [[ ! "$HV_File" =~ \.table$ ]]; then
                Log_Message_Print ERROR HV_Control "HV configuration file must have .table extension."
                return 1
            fi
            while read -r name slot channel vset ohm pmt group; do
                [[ -z "$name" || "$name" == \#* ]] && continue
                if [[ "$vset" -gt "$HV_Max" ]]; then
                    Log_Message_Print ERROR HV_Control "HV value '$vset' exceeds maximum allowed value of ${HV_Max}. (channel: $name)"
                    return 1
                fi
            done < "$HV_File"
            
            shift 2
            ;;
        -command)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR HV_Control "Command is not defined."
                return 1
            fi
            Command="$2"
            
            case "$Command" in
                mon|set|on|off|print) ;;
                *)
                    Log_Message_Print ERROR HV_Control "Invalid command '$Command'. Use mon/set/on/off/print"
                    return 1
                    ;;
            esac
            
            shift 2
            ;;
        *)
            Log_Message_Print ERROR HV_Control "Invalid option: $1"
            return 1
            ;;
    esac
done

if [[ -z "$HV_File" ]] && [[ -z "$Command" ]]; then
    print_usage
    return 1
fi

if [[ -z "$HV_File" ]]; then
    Log_Message_Print ERROR HV_Control "HV configuration file (-hv) is required."
    return 1
fi

if [[ -z "$Command" ]]; then
    Log_Message_Print ERROR HV_Control "Command (-command) is required."
    return 1
fi

if [ "$Command" != "mon" ]; then
    Log_Message_Print INFO HV_Control "${HV_File}  ${Command}"
fi

(
    flock 200
    echo "$HV_File" > /tmp/HV_File
    ./PROGRAM/HVControl/build/hvcontrol  -t "${HV_File}" -c "${Command}"
) 200>/tmp/hv_control.lock

if [ "$Command" == "on" ]; then
Log_Message_Print INFO HV_Control "HV On."
echo 1 > /tmp/is_HV_On
fi

if [ "$Command" == "off" ]; then
Log_Message_Print INFO HV_Control "HV Off."
echo 0 > /tmp/is_HV_On
fi

