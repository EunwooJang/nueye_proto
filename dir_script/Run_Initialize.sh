Name=$1

source dir_script/Parameter.sh
source dir_script/Global.sh

# Initial status parameters

# Run Folder Name
export Run_Folder_Name="$(date +%Y%m%d%H%M%S)__${Name}"

# Device Connection status
echo 0 > /tmp/is_DB_Connected         # USB 3.0 (or PC SSD)
echo 0 > /tmp/is_Internet_Connected
echo 0 > /tmp/is_HV_Board_Connected   # TCP/IP
echo 0 > /tmp/N_TCB                   # USB 3.0
echo 0 > /tmp/N_FADC500               # USB 3.0


# Failure Monitoring
echo 1 > /tmp/is_Tracker_Enabled
# Failure Condition
echo 0 > /tmp/is_CPU_Too_Busy
echo 0 > /tmp/is_CPU_Temp_Too_High
echo 0 > /tmp/is_RAM_Too_Busy
echo 0 > /tmp/is_Memory_Not_Enough
# Failure Flag
echo 0 > /tmp/is_Run_Failure

# HV Related
echo "" > /tmp/HV_File
echo 0 > /tmp/is_HV_On
echo 0 > /tmp/is_HV_Log_Enabled
echo 0 > /tmp/is_HV_Log_Updated
echo 0 > /tmp/is_HV_Off_By_Run_Failure

# DAQ Related
echo 0 > /tmp/is_DAQ_Run_Started
echo "" > /tmp/DAQ_Run_File_Path

# Scripts
echo "" > /tmp/Scheduled_Script_PIDs
echo "" > /tmp/Current_Script_PID

# Select DB
if [[ "$DB_Location_Mode" -eq 1 ]]; then
    export DAQ_Run_DB_Path="$DAQ_Run_DB_Path1"
    export Memory_Disk_Name="$Memory_Disk_Name1"
else
    export DAQ_Run_DB_Path="$DAQ_Run_DB_Path2"
    export Memory_Disk_Name="$Memory_Disk_Name2"
fi

mkdir -p ${Log_Directory} # Make Global Log Directory if not existed
mkdir "${Log_Directory}/${Run_Folder_Name}"

export Log_Folder_Path="${Log_Directory}/${Run_Folder_Name}"

OG_Macro_File="${BASH_SOURCE[1]}"
cp "$OG_Macro_File" $Log_Folder_Path
export Macro_File="${Log_Folder_Path}/$(basename $OG_Macro_File)"

export Run_Log_File="${Log_Folder_Path}/Run_Log.txt"
touch $Run_Log_File

export HV_Log_File="${Log_Folder_Path}/HV_Log.txt"
touch $HV_Log_File

Log_Message_Write INFO Run_Initialize "Start running Macro $OG_Macro_File"
Log_Message_Write INFO Run_Initialize "Macro   File saved   on ${Macro_File}"
Log_Message_Write INFO Run_Initialize "Run Log File created on ${Run_Log_File}"
Log_Message_Write INFO Run_Initialize "HV  Log File created on ${HV_Log_File}"

