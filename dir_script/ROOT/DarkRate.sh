#!/bin/bash

Run_Id=$1

root -l << EOF

// Load File
cout << "${DAQ_Run_File_Path}_${Run_Id}" << endl;


EOF
