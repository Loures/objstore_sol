#!/usr/bin/env bash

WORDS=$(shuf -n50 /usr/share/dict/italian)

for line in $WORDS; do 
    ./test $line 1  >> testout &
    wait $!
done

if [ $(wc -l testout | cut -d" " -f1) -gt 0 ]; then
    rm testout
    echo "Test 1 non passato"
    exit 1
else
    echo "Test 1 passato"
fi

rm testout

for line in $WORDS; do 
    ./test $line 2  >> testout &
    wait $!
done

if [ $(wc -l testout | cut -d" " -f1) -gt 0 ]; then
    cat testout
    rm testout
    echo "Test 2 non passato"
    exit 1
else
    echo "Test 2 passato"
fi

rm testout

for line in $WORDS; do 
    ./test $line 3  >> testout &
    wait $!
done

if [ $(wc -l testout | cut -d" " -f1) -gt 0 ]; then
    cat testout
    rm testout
    echo "Test 3 non passato"
    exit 1
else
    echo "Test 3 passato"
fi

rm testout