if [ "$(cat /tmp/is_HV_Log_Enabled 2>/dev/null || echo 0)" -eq 0 ]; then
    Log_Message_Print ERROR HV_Log_End "HV Logging not started yet."
    return 1
fi

echo 0 > /tmp/is_HV_Log_Enabled

sleep 6

Log_Message_Print INFO HV_Log "HV logging stopped."
Log_Message_Print INFO HV_Monitor "Stop HV stopped."
Log_Message_Print_Empty

