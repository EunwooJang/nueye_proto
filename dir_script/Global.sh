Log_Message_Print() {
    local Level=$1
    shift
    local Script_Name=$1
    shift
    local Message="$*"
    local Timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    local Line="${Timestamp} [${Level}] ${Script_Name}:  ${Message}"

    echo "${Line}"
}

Log_Message_Print_Empty() {
    echo ""
}

Log_Message_Write() {
    local Level=$1
    shift
    local Script_Name=$1
    shift
    local Message="$*"
    local Timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    local Line="${Timestamp} [${Level}] ${Script_Name}:  ${Message}"

    echo "${Line}"

    if [ -f "$Run_Log_File" ]; then
        echo "${Line}" >> "$Run_Log_File"
    fi
}

Log_Message_Write_Empty(){
    echo ""
    if [ -n "$Run_Log_File" ]; then
        echo "" >> "$Run_Log_File"
    fi
}
