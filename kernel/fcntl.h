#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
// MT: create the file if it does not exist. If it already exists, open it does not erase its contents
#define O_TRUNC   0x400
// MT: truncate an existing regular file to zero bytes when opened
