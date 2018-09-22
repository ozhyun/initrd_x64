#
# Initrd Makefile
#

ifndef INITRD_DIR
    INITRD_DIR = $(PWD)/initrd_tmp
    export INITRD_DIR
endif

kernel_dir=../../linux-4.14.70
driver_dir=initrd_tmp/lib/modules/kernel/drivers/net/

all: pre drivers
	cd initrd_tmp && find . | cpio -o -c > ../initrd.cpio
	gzip -nf initrd.cpio && mv initrd.cpio.gz initrd.img

pre:
	rm -rf initrd_tmp
	cp -rf initrd initrd_tmp
	cd initrd_tmp && tar xzf ../dev.tar.gz
	cd initrd_tmp && rm -rf `find -name ".svn"`
	#cd src && make install

drivers:
	mkdir -p $(driver_dir)
	cp -f $(kernel_dir)/drivers/net/ethernet/intel/e1000/e1000.ko $(driver_dir)
	cp -f $(kernel_dir)/drivers/net/ethernet/intel/e1000e/e1000e.ko $(driver_dir)
	cp -f $(kernel_dir)/drivers/net/ethernet/intel/igb/igb.ko $(driver_dir)
	cp -f $(kernel_dir)/drivers/net/ethernet/intel/ixgbe/ixgbe.ko $(driver_dir)
	cp -f $(kernel_dir)/drivers/net/ethernet/intel/i40e/i40e.ko $(driver_dir)
	cp -f $(kernel_dir)/drivers/net/ethernet/amd/pcnet32.ko $(driver_dir)
	cp -f $(kernel_dir)/drivers/net/mii.ko $(driver_dir)
	cp -f $(kernel_dir)/drivers/net/mdio.ko $(driver_dir)
	cp -f $(kernel_dir)/drivers/dca/dca.ko $(driver_dir)

env:
	cd src && make $@

clean:
	cd src && make $@
	rm -rf initrd_tmp
	rm -rf initrd.img
	
