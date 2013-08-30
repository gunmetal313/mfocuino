Arduino + NFc shield= NFC reader/writer


sudo apt-get install subversion dpkg-dev debhelper dh-autoreconf libtool

sudo apt-get install libuсsb-dev libpcsclite-dev

sudo apt-get install libusb-0.1-4 libpcsclite1 libccid pcscd

wget http://libnfc.googlecode.com/files/libnfc-1.6.0-rc1.tar.gz

tar -xvzf libnfc-1.6.0-rc1.tar.gz

cd libnfc-1.6.0-rc1/

patch -p0 < libnfc.uart.patch

./configure --with-drivers=pn532_uart --sysconfdir=/etc --prefix=/usr --enable-serial-autoprobe

make clean all

sudo make install all

ldconfig

sudo mkdir -p /etc/nfc/

sudo mkdir -p /etc/nfc/devices.d

sudo cp /libnfc-1.7.0-rc7/contrib/libnfc/pn532_via_uart2usb.conf.sample /usr/local/etc/nfc/devices.d/pn532_via_uart2usb.conf

nfc-scan-device -i -v

nfc-list
