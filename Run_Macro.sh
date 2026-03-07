#!/bin/bash

Macro_Path=$1
Name=${2:-Test}

if [[ -z "$Macro_Path" ]]; then
    echo "Usage: RUN.sh <Macro_Path> [Name]"
    return 1
fi

# Open New terminal Window without showing
tmux new-session -d -s $Name -x 220 -y 50

# Wait for terminal stabled
sleep 1

# Send command to run below script
tmux send-keys -t $Name ". dir_script/Script_Wrapper.sh $Macro_Path $Name; tmux kill-session -t $Name" Enter

# Need for terminal stable
sleep 1

# Print out terminal name 
tmux ls

# Report User when those terminal ended
(
    while true; do
        sleep 3
        if ! tmux has-session -t $Name 2>/dev/null; then
            echo "$(date '+%Y-%m-%d %H:%M:%S') [INFO] Session $Name is finished"
            break
        fi
    done
) & disown

