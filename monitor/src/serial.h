#ifndef SERIAL_H
#define SERIAL_H

#define DEFAULT_SERIAL_PORT "/dev/ttyACM0"
#define DEFAULT_BAUDRATE B115200

int serial_open(const char *port);
void serial_close(int fd);
int serial_write_data(int fd, const char *data, int len);

#endif
