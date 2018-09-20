#
# Makfile to create bootfs
#
ifndef TARGET
	TARGET = $(PWD)/Target
endif

IMAGE = bootfs.pkg

all: pre
	cd initrd && make $@ 
	cp initrd/initrd.img version/$(IMAGE)
	cp syslinux/syslinux.cfg version
	cp syslinux/3.86/win32/syslinux.exe version
	cp initrd/package.cfg version
	cp howto version

.PHONY: pre clean

pre:
	mkdir -p version
	cd initrd && make $@

env:
	cd initrd && make $@

install: all
	mkdir -p $(TARGET)
	cp -f version/$(IMAGE) $(TARGET)/$(IMAGE)
	cp -f version/syslinux.cfg $(TARGET)
	cp -f version/package.cfg $(TARGET)

clean:
	cd initrd && make clean
	rm -rf version
	rm -rf $(TARGET)
