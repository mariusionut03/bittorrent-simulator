build:
	mpic++ -o bittorrent bittorrent.cpp -pthread -Wall

clean:
	rm -rf bittorrent

run:
	mpirun --oversubscribe -np 4 ./bittorrent

clear-tests:
	rm -r tests/input/*
	rm -r tests/output/*