all:
	gcc -g -I/usr/include/libnl3 -I/usr/include/json-c -lnl-3 -lnl-genl-3 -ljson-c -lcurl -o clocate main.c nl80211.c mozilla.c google.c provider.c