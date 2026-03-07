SCRIPT_CMD="$1"
DELAY="$2"

Log_Message_Write INFO Script_Scheduler "Script ${SCRIPT_CMD} is scheduled"

(

sleep "$DELAY"

Log_Message_Write INFO Script_Scheduler "Script ${SCRIPT_CMD} is started"
eval "$SCRIPT_CMD" 2>&1 | tee -a "${Run_Log_File}" # Sorry.. I don't want to use this dangerous codes either.
Log_Message_Write INFO Script_Scheduler "Script ${SCRIPT_CMD} is ended"

sed -i "/^${PID}$/d" /tmp/Scheduled_Script_PIDs
) &

PID=$!

disown

echo "$PID" >> /tmp/Scheduled_Script_PIDs
