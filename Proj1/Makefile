all: data-transfer

data-transfer: app_layer.c link_layer.c link_layer.h defs.h
	gcc -Wall app_layer.c link_layer.c -o file_transfer 

clean:
	rm -f file_transfer
