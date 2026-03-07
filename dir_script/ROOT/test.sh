
text=$1

root -l << EOF
cout << "$text" << endl;
EOF
