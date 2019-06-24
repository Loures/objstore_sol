#!/usr/bin/env bash

if [ ! -e "/tmp/objstore.sock" ]; then
    echo "Server non attivo"
    exit 1
fi

WORDS=$(shuf -n50 /usr/share/dict/italian)
rm testout.log 2> /dev/null

function printstats {
	TEST1=$(grep -c "Test 1 passato" testout.log)
	TEST2=$(grep -c "Test 2 passato" testout.log)
	TEST3=$(grep -c "Test 3 passato" testout.log)
	echo "Test 1 passati: $TEST1"
	echo "Test 1 falliti: $((50 - $TEST1))"
	echo "Test 2 passati: $TEST2"
	echo "Test 2 falliti: $((30 - $TEST2))"
	echo "Test 3 passati: $TEST3"
	echo "Test 3 falliti: $((20 - $TEST3))"
}

function testclient {
    ./client $1 $2 > /dev/null &
    wait $!
    if [ $? -eq 1 ]; then
        echo "Client PID $!: Test $2 non passato" >> testout.log
        return 1
    else
        echo "Client PID $!: Test $2 passato" >> testout.log
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
wait
sleep .1
pkill -TERM os_server

printstats
