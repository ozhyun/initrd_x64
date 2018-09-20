####################################################
# This script was used to clean up harddisk
# Now, remove configuration & license
####################################################
#!/bin/sh
#Delete file in /etc & /sbin
rootdir="/sysroot"
[ $# -eq 1 ] && {
	rootdir=$1
}

rm -rf $rootdir/lib $rootdir/usr/lib
rm -rf $rootdir/etc/
rm -rf $rootdir/sbin/

#Delete old config file by wangkai
if [ -d $rootdir/var/etc ];then
	rm -rf $rootdir/var/etc
fi

#Delete pid files
if [ -d $rootdir/var/run ];then
	rm -f $rootdir/var/run/*.pid
fi

if [ -d $rootdir/extra ];then
	rm -rf $rootdir/extra
fi
#delete softlink file
rm -rf $rootdir/usr/local/lib/php
#rm -rf $rootdir/usr/local/etc
rm -rf $rootdir/usr/local/www/portal
rm -f $rootdir/usr/local/www/data
rm -rf $rootdir/usr/local/www/conf
#rm -rf $rootdir/usr/local/www/portal.ambit/ui
#rm -rf $rootdir/usr/local/www/data.ambit/ui
rm -rf $rootdir/tmp/*
rm -f $rootdir/etc/cron.d/*
rm -f $rootdir/usr/local/etc/logcfg/*
#cleanup old gram directory
if [ -d /InstallGram/ ];then
    rm -rf $rootdir/InstallGram/*
fi

echo "System Cleanup Finished"

