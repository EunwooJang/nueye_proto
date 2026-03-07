
if [ "$(cat /tmp/N_TCB 2>/dev/null || echo 0)" -eq 0 ]; then
    Log_Message_Print ERROR DAQ_Run_End "TCB Board is not connected."
    return 1
fi

if [ "$(cat /tmp/N_FADC500 2>/dev/null || echo 0)" -eq 0 ]; then
    Log_Message_Print ERROR DAQ_Run_End "FADC500 is not connected."
    return 1
fi

touch /tmp/forced.endrun

sleep 10
sudo /usr/local/bin/reset_usb.sh
Log_Message_Print INFO DAQ_Run_End "Run terminated."
echo 0 > /tmp/is_DAQ_Run_Started

