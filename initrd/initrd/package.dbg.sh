###########################################
# Load License & packages
#
##########################################
#!/bin/sh

release_license()
{
    hwdir="/sysroot/extra"
    pubdir="/sysroot/usr/local/etc/ambit"
    phpdir="/sysroot/usr/local/lib/php/vni_ui"
    license="/mnt/license"
    mkdir -p $hwdir
    mkdir -p $pubdir
    mkdir -p $phpdir
    tar zxf $license extra/license -C /sysroot 1>/dev/null 2>&1 
    tar zxf $license extra/features -C /sysroot 1>/dev/null 2>&1 
    tar zxf $license extra/oem_hostname -C /sysroot 1>/dev/null 2>&1 
    tar zxf $license usr/local/etc/ambit/feature.pub -C /sysroot 1>/dev/null 2>&1 
    tar zxf $license usr/local/lib/php/vni_ui/vni_hw_constants.inc.php -C /sysroot 1>/dev/null 2>&1 
}
#directory for local statics
mkdir -p /sysroot/data/txtdb/localads
chown -R 501:501 /sysroot/data/txtdb 1>/dev/null 2>&1

#Load license
#check devcice be produced or not
echo "Checking produce status"
if [ -f /mnt/license ]; then
	if [ -f /mnt/produce.rls ]; then
		rm -f /mnt/produce.rls
		echo "Remove produce pkg success"
	fi

	if [ -f /mnt/produce.pkg ]; then
		rm -f /mnt/produce.pkg
		echo "Remove produce pkg"
	fi

	#tar zxf /mnt/license -C /sysroot
    release_license
	echo "Load license success"
elif [ -f /mnt/produce.pkg ]; then
	/bin/expkg extract /mnt/produce.pkg /sysroot
	echo "Load produce pkg success"
#elif [ -f /mnt/produce.rls ]; then
#	tar zxf /mnt/produce.rls -C /sysroot
#	echo "Loaded produce rls success"
else
	echo "Cann't find produce package"
fi

# Get device type from features
# default was VSM
DEVTYPE="VSM"
if [ -f /sysroot/extra/features ]; then
while read TYPE args
do
	case "$TYPE" in 
		\#*|"") continue ;;
	esac

	if [ "$TYPE" = "control-server" ]; then
		DEVTYPE="CS"
		break
	fi
done < /sysroot/extra/features
fi

# according device type select package.cfg
if [ "$DEVTYPE" = "CS" -a -f /mnt/package.cfg.cs ]; then
	echo "Use package.cfg for CS/CAM"
	cp -f /mnt/package.cfg /mnt/package.cfg.ini
	mv -f /mnt/package.cfg.cs /mnt/package.cfg
fi

#Load Package 
if [ -f /mnt/package.cfg ]
then
	(cat /mnt/package.cfg; echo) | # make sure there is a LF at the end
	while read package args
	do
		case "$package" in
			\#*|"") continue ;;
		esac
		if [ -f /mnt/$package.pkg ]; then
			echo "Loading $package.pkg ..."
			/bin/expkg extract /mnt/$package.pkg /sysroot
		elif [ -f /mnt/$package.rls ];then
			echo "Loading $package.rls ..."
			tar zxf /mnt/$package.rls -C /sysroot
		else
			echo "Error: $package not exist or invalid pkg"
		fi

	done
else
	echo "Can't find package.cfg"
fi

#Load version package
if [ -f /mnt/release-version.rls ];then
	tar zxf /mnt/release-version.rls -C /sysroot
fi

# Init DB for CS or CAM
init_db()
{
	DBRLS=release-database-r`grep "database" /mnt/version | awk -F' ' '{print $2}'`
	echo $DBRLS >> /mnt/db.log
	if [ -f /mnt/$DBRLS.pkg ]; then
		echo "Init DB with $DBRLS.pkg"
		/bin/expkg extract /mnt/$DBRLS.pkg  /sysroot
		echo "1" >/sysroot/data/reload_db_config 
	elif [ -f /mnt/$DBRLS.rls ]; then
		echo "Init DB with $DBRLS.rls"
		tar zxf /mnt/$DBRLS.rls -C /sysroot
		echo "1" >/sysroot/data/reload_db_config 
	else
		echo "DB package not exist!" >> /mnt/db.log
	fi
}

if [ "$DEVTYPE" = "CS" ];then
	if [ -f /sysroot/data/mysql/version ];then
		run_db_ver=$(cat /sysroot/data/mysql/version)
	else
		run_db_ver=""
	fi
	new_db_ver=$(grep "database" /mnt/version | awk -F' ' '{print $2}')
fi

if [ "$DEVTYPE" = "CS" -a ! -d /sysroot/data/mysql/r2 -a -f /mnt/version ]; then
	echo "`date` db isn't exist,need init" >>/mnt/db.log
	init_db
elif [ "$DEVTYPE" = "CS" -a "$run_db_ver" != "$new_db_ver" ];then
	echo "db version isn't match,need init" >>/mnt/db.log
	init_db
elif [ "$DEVTYPE" = "CS" -a -d /sysroot/data/mysql/r2 -a -f /sysroot/data/reload_db_config ];then
	rm -f /sysroot/data/reload_db_config
	echo "don't need to reload db" >> /mnt/db.log
fi

# Load hardware config
if [ -f /mnt/hwconf.rls ]; then
    mkdir -p /sysroot/tmp
	tar zxf /mnt/hwconf.rls extra/devslot -C /sysroot/
fi

if [ -f /mnt/hwconf.pkg ]; then
	/bin/expkg extract /mnt/hwconf.pkg  /sysroot
fi

if [ -f /mnt/hwconf ]; then
	/bin/expkg extract /mnt/hwconf  /sysroot
fi

#Load config
mkdir -p /sysroot/var/etc
if [ "$DEVTYPE" = "CS" ];then
    rm -f /sysroot/var/etc/*.sql 
fi

if [ -f /mnt/config.tgz ]; then
	tar zxf /mnt/config.tgz -C /sysroot/var/etc
	# remove config.vamd.* in CS or IS, and remove config.vcsd.* & *.sql in VSM
	if [ "$DEVTYPE" = "CS" ]; then
		rm -f /sysroot/var/etc/config.vamd.*
	else
		rm -f /sysroot/var/etc/config.vcsd.*
		rm -f /sysroot/var/etc/*.sql
	fi

	if [ -f /mnt/config_db.tgz ]; then
		tar zxf /mnt/config_db.tgz -C /sysroot/var/etc
	fi
	echo "Load config success"
elif [ -f /mnt/factory_config.tgz ]; then
	tar zxf /mnt/factory_config.tgz -C /sysroot/var/etc
	echo "Not exist config, Reset config to factory"
else
	echo "Cann't find config & factory_config"
fi

#Load license
#check devcice be produced or not
echo "Checking produce status"
if [ -f /mnt/license ]; then
	#tar zxf /mnt/license -C /sysroot
    release_license
	echo "Load license success"
fi

#init directory rights
chown -R 501:501 /sysroot/usr/local/lib/php 1>/dev/null 2>&1
chown -R 501:501 /sysroot/usr/local/www 1>/dev/null 2>&1
chmod -R 744 /sysroot/usr/local/www/data* 1>/dev/null 2>&1
chmod -R 744 /sysroot/usr/local/www/portal* 1>/dev/null 2>&1
chmod -R 744 /sysroot/usr/local/www/wsdl 1>/dev/null 2>&1
chmod -R 744 /sysroot/usr/local/lib/php 1>/dev/null 2>&1 
chmod -R +x /sysroot/var/vdist/* 1>/dev/null 2>&1
#mysql:mysql chown change
chown -R 27:27 /sysroot/data/mysql 1>/dev/null 2>&1
#mod by tony.xu
mkdir -p /sysroot/usr/local/etc/logcfg/
mkdir -p /sysroot/etc/cron.d/

if [ -f /mnt/bootfs.rls ];then
    rm -f /mnt/bootfs.rls
fi
if [ -f /mnt/rootfs.rls ];then
    rm -f /mnt/rootfs.rls
fi

echo "Loaded all nedded pkgs"
