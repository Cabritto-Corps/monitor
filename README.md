# monitor
literalmente o nome do repositorio

cd /esp
. ~/esp/esp-idf/export.sh
sudo chmod a+rw /dev/ttyACM0
idf.py -p /dev/ttyACM0 build
idf.py -p /dev/ttyACM0 flash
