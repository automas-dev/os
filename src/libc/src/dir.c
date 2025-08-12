#include "libc/dir.h"

#include "libk/sys_call.h"

dir_t dir_open(const char * path) {
    return _sys_io_dir_open(path);
}

void dir_close(dir_t dp) {
    _sys_io_dir_close(dp);
}

int dir_read(dir_t dp, dir_entry_t * dir_entry) {
    return _sys_io_dir_read(dp, dir_entry);
}

int dir_seek(dir_t dp, int offset, int origin) {
    return _sys_io_dir_seek(dp, offset, origin);
}

int dir_tell(dir_t dp) {
    return _sys_io_dir_tell(dp);
}
