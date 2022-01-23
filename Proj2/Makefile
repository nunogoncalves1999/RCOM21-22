all: download_app

download_app: ftp_download.c defs.h
	gcc ftp_download.c -o download 

clean:
	rm -f download
