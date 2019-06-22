#!/usr/bin/env bash

if [ ! -e "/tmp/objstore.sock" ]; then
    echo "Server non attivo"
    exit 1
fi

WORDS=$(shuf -n50 /usr/share/dict/italian)

function testclient {
    ./test $1 $2 > /dev/null &
    wait $!
    if [ $? -eq 1 ]; then
        echo "Client PID $!: Test $2 non passato"
        return 1
    else
        echo "Client PID $!: Test $2 passato"
    fi
}

for line in $WORDS; do 
    testclient $line 1 &  
    wait
done

for line in $(head -30 <<< "$WORDS"); do 
    testclient $line 2 &
done


for line in $(tail -20 <<< "$WORDS"); do 
    testclient $line 3 &
done
wait

pkill -USR1 os_server
