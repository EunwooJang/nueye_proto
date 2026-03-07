
USER_SCRIPT="$1"
Name="$2"

. dir_script/Run_Initialize.sh "$Name"

. dir_script/Tracker_Start.sh


PROCESSED_SCRIPT=$(mktemp /tmp/processed_XXXXXX.sh)

while IFS= read -r line; do

    trimmed="${line#"${line%%[![:space:]]*}"}"

    if [[ "$trimmed" =~ ^\:\ \<\<[\'\"]*([A-Z]+)[\'\"]*$ ]]; then
        HEREDOC_END="${BASH_REMATCH[1]}"
        echo "$line"
        while IFS= read -r line; do
            echo "$line"
            if [[ "$line" == "$HEREDOC_END" ]]; then
                break
            fi
        done
        continue
    fi

    if [[ -z "$trimmed" ]] || [[ "$trimmed" == \#* ]]; then
        echo "$line"
    elif [[ "$trimmed" =~ ^\) ]] || [[ "$trimmed" =~ ^[a-zA-Z_]+=\( ]] || [[ "$trimmed" =~ ^[0-9] ]]; then
        echo 'if [[ "$(cat /tmp/is_Run_Failure 2>/dev/null || echo 0)" -eq 0 ]]; then'
        echo "Log_Message_Write_Empty"
        echo "Log_Message_Write INFO Script \"Run: $trimmed\""
        echo "$trimmed"
        echo "Log_Message_Write INFO Script \"Run: $trimmed\""

        echo 'fi'
    elif [[ "$trimmed" =~ Script_Scheduler ]]; then
        echo 'if [[ "$(cat /tmp/is_Run_Failure 2>/dev/null || echo 0)" -eq 0 ]]; then'
        echo 'Log_Message_Write_Empty'
        printf "    Log_Message_Write INFO Script 'Start script: %s'\n" "$trimmed"
        printf '    %s\n' "$trimmed"
        printf "    Log_Message_Write INFO Script 'End script: %s'\n" "$trimmed"
        echo 'fi'
    else
        echo 'if [[ "$(cat /tmp/is_Run_Failure 2>/dev/null || echo 0)" -eq 0 ]]; then'
        echo 'Log_Message_Write_Empty'
        printf "    Log_Message_Write INFO Script 'Start script: %s'\n" "$trimmed"
        echo "    echo \$\$ > /tmp/Current_Script_PID"
        printf '    { %s ; } 2>&1 | tee -a "${Run_Log_File}"\n' "$trimmed"
        printf "    Log_Message_Write INFO Script 'End script: %s'\n" "$trimmed"
        echo 'fi'
    fi

done < "$USER_SCRIPT" > "$PROCESSED_SCRIPT"

source "$PROCESSED_SCRIPT"
rm "$PROCESSED_SCRIPT"


if [[ "$(cat /tmp/is_HV_Off_By_Run_Failure 2>/dev/null || echo 0)" -eq 0 ]]; then
    sleep 30
fi

if [ "$(cat /tmp/is_HV_Log_Enabled 2>/dev/null || echo 0)" -eq 1 ]; then
    Log_Message_Write WARNING HV_Log "HV Logging stop not declared on your macro. Auto terminated."
    . dir_script/HV_Log_End.sh
    return 1
fi

. dir_script/Tracker_End.sh

. dir_script/Run_Finalize.sh

