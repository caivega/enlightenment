/*

Copyright (C) 2000, 2001 Christian Kreibich <cK@whoop.org>.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#ifndef efsd_commands_h
#define efsd_commands_h

#include <efsd.h>

int  efsd_command_remove(EfsdCommand *cmd, int client);
int  efsd_command_move(EfsdCommand *cmd, int client);
int  efsd_command_copy(EfsdCommand *cmd, int client);
int  efsd_command_symlink(EfsdCommand *cmd, int client);
int  efsd_command_makedir(EfsdCommand *cmd, int client);
int  efsd_command_chmod(EfsdCommand *cmd, int client);
int  efsd_command_set_metadata(EfsdCommand *cmd, int client);
int  efsd_command_get_metadata(EfsdCommand *cmd, int client);
int  efsd_command_listdir(EfsdCommand *cmd, int client);
int  efsd_command_start_monitor_file(EfsdCommand *cmd, int client);
int  efsd_command_start_monitor_dir(EfsdCommand *cmd, int client);
int  efsd_command_stop_monitor(EfsdCommand *cmd, int client);
int  efsd_command_stat(EfsdCommand *cmd, int client, char use_lstat);
int  efsd_command_readlink(EfsdCommand *cmd, int client);
int  efsd_command_get_filetype(EfsdCommand *cmd, int client);

#endif
