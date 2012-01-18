// main.cpp is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

#include "mpq_fs.h"

int main (int argc, char** argv)
{
  assert ("application [fuse options] <mountpoint> <mpq>" && argc >= 2);
  return fuse::start_file_system (argc - 1, argv, new mpq_fs (argv[argc - 1]));
}
