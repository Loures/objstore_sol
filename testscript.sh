#!/usr/bin/env bash

if [ ! -e "/tmp/objstore.sock" ]; then
    echo "Server non attivo"
    exit 1
fi

WRITE=$1
READ=$(($1 * 3/5));
DELETE=$(($1 * 2/5))

WORDS=$(shuf -n$WRITE /usr/share/dict/italian)
rm testout.log 2> /dev/null

function printstats {
	TEST1=$(grep -c "Test 1 passato" testout.log)
	TEST2=$(grep -c "Test 2 passato" testout.log)
	TEST3=$(grep -c "Test 3 passato" testout.log)
	echo "Test 1 passati: $TEST1"
    grep "Test 1 non passato" testout.log
	echo "Test 1 non passati: $(($WRITE - $TEST1))"
	echo "Test 2 passati: $TEST2"
    grep "Test 2 non passato" testout.log
	echo "Test 2 non passati: $(($READ - $TEST2))"
	echo "Test 3 passati: $TEST3"
    grep "Test 3 non passato" testout.log
	echo "Test 3 non passati: $(($DELETE - $TEST3))"
}

function testclient {
    ./client $1 $2 > /dev/null
    if [ $? -eq 1 ]; then
        echo "Client '$1': Test $2 non passato" >> testout.log
        return 1
    else
        echo "Client '$1': Test $2 passato" >> testout.log
    fi
}

for line in $WORDS; do 
    testclient $line 1 &  
done
wait

for line in $(head -$READ <<< "$WORDS"); do 
    testclient $line 2 &
done


for line in $(tail -$DELETE <<< "$WORDS"); do 
    testclient $line 3 &
done
wait

pkill -USR1 os_server
sleep .25
pkill -TERM os_server

printstats
