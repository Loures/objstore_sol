#!/usr/bin/env bash

if [ ! -e "/tmp/objstore.sock" ]; then
    echo "Server non attivo"
    exit 1
fi


WORDS=$(shuf -n50 /usr/share/dict/italian)

function testclient {
    ./test $1 $2 &
    wait $!
    if [ -f $? ]; then
        echo "Client PID $!: Test $2 non passato"
        return 1
    else
        echo "Client PID $!: Test $2 passato"
    fi
}

#for line in $WORDS; do 
#    ./test $line 1  >> testout &
#    wait $!
#done
#
#if [ $(wc -l testout | cut -d" " -f1) -gt 0 ]; then
#    rm testout
#    echo "Test 1 non passato"
#    exit 1
#else
#    echo "Test 1 passato"
#fi
#
#rm testout

for line in $WORDS; do 
    testclient $line 1  
done

for line in $WORDS; do 
    testclient $line 2  
done

for line in $WORDS; do 
    testclient $line 3 
done

