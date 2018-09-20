#
# Initrd Makefile
#

ifndef INITRD_DIR
    INITRD_DIR = $(PWD)/initrd_tmp
    export INITRD_DIR
endif

all: pre
	cd initrd_tmp && find . | cpio -o -c > ../initrd.cpio
	gzip -nf initrd.cpio && mv initrd.cpio.gz initrd.img

pre:
	cp -rf initrd initrd_tmp
	cd initrd_tmp && tar xzf ../dev.tar.gz
	cd initrd_tmp && rm -rf `find -name ".svn"`
	cd src && make install

env:
	cd src && make $@

clean:
	cd src && make $@
	rm -rf initrd_tmp
	rm -rf initrd.img
	
