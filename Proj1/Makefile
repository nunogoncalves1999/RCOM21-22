all: data-transfer

data-transfer: app_layer.c link_layer.o defs.h
	gcc -Wall app_layer.c -o data_transfer 
	
link_layer.o: link_layer.c link_layer.h defs.h
	gcc -Wall -c link_layer.c -o link_layer.o 

clean:
	rm -f data_transfer link_layer.o
