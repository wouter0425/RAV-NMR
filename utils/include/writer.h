#ifndef WRITER_H
#define WRITER_H

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <string>

using namespace std;

int create_directory(string &path);

#endif