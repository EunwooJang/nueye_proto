
print_usage() {
    echo ""
    echo "Usage:   . HV_Create_Config.sh -hv [Input HV configuration file]  -output [Output HV configuration file] -value [value1] [value2] ... -apply"
    echo "Example: . HV_Create_Config.sh -hv dir_config/HV/HV_default.table -output HV_1500.table -value 1500 1500 1500 -apply"
    echo "         . HV_Create_Config.sh -hv dir_config/HV/HV_default.table -output HV_1500.table"
    echo "         . HV_Create_Config.sh -hv dir_config/HV/HV_default.table -output HV_1500.table -value s s s 1500"
    echo ""
    echo "options:"
    echo "  -hv     Input  HV configuration file"
    echo "  -output Output HV configuration file"
    echo "  -value  HV values to switch (Applied by line order, use 's' to skip/keep original value, optional)"
    echo "  -apply  Apply output HV configuration file to current HV Board (optional)"
    echo ""
}

HV_File=""
Out_File=""
Apply_Mode=false

Values=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        -hv)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR HV_Create_Config "HV configuration file is not defined."
                return 1
            fi
            HV_File="$2"
            
            if [[ ! -f "$HV_File" ]]; then
                Log_Message_Print ERROR HV_Create_Config "File '$HV_File' not found."
                return 1
            fi
            
            if [[ ! "$HV_File" =~ \.table$ ]]; then
                Log_Message_Print ERROR HV_Create_Config "HV config file must have .table extension."
                return 1
            fi
            
            shift 2
            ;;
        -output)
            if [[ -z "$2" ]]; then
                Log_Message_Print ERROR HV_Create_Config "Output filename is not defined."
                return 1
            fi
            Out_File="$2"
            
            if [[ ! "$Out_File" =~ \.table$ ]]; then
                Log_Message_Print ERROR HV_Create_Config "Output file must have .table extension."
                return 1
            fi
            
            shift 2
            ;;
        -value)
            shift
            while [[ $# -gt 0 && "$1" != -* ]]; do
                if [[ "$1" == "s" ]]; then
                    Values+=("s")
                elif ! [[ "$1" =~ ^[0-9]+$ ]]; then
                    Log_Message_Print ERROR HV_Create_Config "HV value must be integer or 's' (skip): '$1' is not valid."
                    return 1
                else
                    if [[ "$1" -gt $HV_Max ]]; then
                        Log_Message_Print ERROR HV_Create_Config "HV value '$1' exceeds maximum allowed value of ${HV_Max}."
                        return 1
                    fi
                    Values+=("$1")
                fi
                shift
            done
            ;;
        -apply)
            Apply_Mode=true
            shift
            ;;
        *)
            Log_Message_Print ERROR HV_Create_Config "Invalid option: $1"
            return 1
            ;;
    esac
done

if [[ -z "$HV_File" ]] && [[ -z "$Out_File" ]]; then
    print_usage
    return 1
fi

if [[ -z "$HV_File" ]]; then
    Log_Message_Print ERROR HV_Create_Config "Input file (-hv) is required."
    return 1
fi

if [[ -z "$Out_File" ]]; then
    Log_Message_Print ERROR HV_Create_Config "Output file (-output) is required."
    return 1
fi


if [[ ${#Values[@]} -eq 0 ]]; then
    cp "$HV_File" "$Out_File"
    Log_Message_Print INFO HV_Create_Config "No value defined. Just Copy&Paste operated."
    Log_Message_Print INFO HV_Create_Config "HV table created: $Out_File"
else
    Line_Num=0
    HV_Index=3

    > "$Out_File"

    while read -r line; do
        read -r -a cols <<< "$line"

        if [[ $Line_Num -lt ${#Values[@]} ]]; then
            if [[ "${Values[$Line_Num]}" != "s" ]]; then
                cols[$HV_Index]="${Values[$Line_Num]}"
            fi
        fi

        printf "%s" "${cols[0]}" >> "$Out_File"
        for ((i=1; i<${#cols[@]}; i++)); do
            printf "\t%s" "${cols[$i]}" >> "$Out_File"
        done
        printf "\n" >> "$Out_File"

        ((Line_Num++))
    done < "$HV_File"

    Log_Message_Print INFO HV_Create_Config "HV table created: $Out_File"
fi

if $Apply_Mode; then
    if [ "$(cat /tmp/is_HV_Board_Connected 2>/dev/null || echo 0)" -eq 0 ]; then
        Log_Message_Print ERROR HV_Create_Config "HV Board is not connected."
        return 1
    fi

    (
    flock 200
    Log_Message_Print INFO HV_Create_Config "Applying HV settings to board..."
    echo "$Out_File" > /tmp/HV_File
    ./PROGRAM/HVControl/build/hvcontrol -t "$Out_File" -c set
    ) 200>/tmp/hv_control.lock
fi

