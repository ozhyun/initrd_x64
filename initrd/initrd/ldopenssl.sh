#!/bin/sh
#
# Script used to load openssl & connective libs into root
#

cp -af /bin/openssl               /sysroot/usr/bin/
cp -af /lib/libcom_err.so.2       /sysroot/usr/lib/
cp -af /lib/libcom_err.so.2.1     /sysroot/usr/lib/
cp -af /lib/libcrypto.so.0.9.8e   /sysroot/usr/lib/
cp -af /lib/libcrypto.so.6        /sysroot/usr/lib/
cp -af /lib/libgssapi_krb5.so     /sysroot/usr/lib/
cp -af /lib/libgssapi_krb5.so.2   /sysroot/usr/lib/
cp -af /lib/libgssapi_krb5.so.2.2 /sysroot/usr/lib/
cp -af /lib/libk5crypto.so        /sysroot/usr/lib/
cp -af /lib/libk5crypto.so.3      /sysroot/usr/lib/
cp -af /lib/libk5crypto.so.3.1    /sysroot/usr/lib/
cp -af /lib/libkeyutils-1.2.so    /sysroot/usr/lib/
cp -af /lib/libkeyutils.so.1      /sysroot/usr/lib/
cp -af /lib/libkrb5.so            /sysroot/usr/lib/
cp -af /lib/libkrb5.so.3          /sysroot/usr/lib/
cp -af /lib/libkrb5.so.3.3        /sysroot/usr/lib/
cp -af /lib/libkrb5support.so     /sysroot/usr/lib/
cp -af /lib/libkrb5support.so.0   /sysroot/usr/lib/
cp -af /lib/libkrb5support.so.0.1 /sysroot/usr/lib/
cp -af /lib/libresolv-2.5.so      /sysroot/lib/
cp -af /lib/libresolv.so.2        /sysroot/lib/
cp -af /lib/libselinux.so.1       /sysroot/lib/
cp -af /lib/libsepol.so.1         /sysroot/lib/
cp -af /lib/libssl.so.0.9.8e      /sysroot/usr/lib/
cp -af /lib/libssl.so.6           /sysroot/usr/lib/
cp -af /lib/libz.so               /sysroot/usr/lib/
cp -af /lib/libz.so.1             /sysroot/usr/lib/
cp -af /lib/libz.so.1.2.3         /sysroot/usr/lib/

echo "Install SSL Finished"

