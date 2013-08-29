nfc-scan-device -i -v


sudo mkdir -p /etc/nfc/devices.d

./configure --with-drivers=pn532_uart --sysconfdir=/etc --prefix=/usr
make clean all
sudo make install all 

./configure --with-drivers=pn532_uart --sysconfdir=/etc --prefix=/usr

./configure --with-drivers=pn532_uart --enable-serial-autoprobe (--enable-debug)


patch -p0 < libnfc.uart.patch


