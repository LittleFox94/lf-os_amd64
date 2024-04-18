int read_lfos(int file, char *ptr, int len);
int write_lfos(int file, char *ptr, int len);

int read(int file, char *ptr, int len) {
    return read_lfos(file, ptr, len);
}

int write(int file, char *ptr, int len) {
    return write_lfos(file, ptr, len);
}
