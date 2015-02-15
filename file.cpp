#include "file.h"
#include "folder.h"
#include "serialize.h"
#include "crc32.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <stdexcept>

using namespace std;

File::File(const Folder *parent, const std::string& Path)
    : parent{parent},
      rawSize{0}, actualSize{0},
      crc32{0}, attrs{0,0,0,0}
{
    path = Path;
    readAttributes();
    computeCRC();
}

File::File(const Folder *parent, const std::vector<char>& data)
    : parent{parent}
{
    deserialize(data);
}

void File::readAttributes()
{
    struct stat buf;
    if (stat((parent->getPath()+"/"+path).c_str(), &buf) < 0)
    {
        cerr << "WARNING: File::readAttributes: Failed to stat "<<path<<endl;
        return;
    }

    rawSize = actualSize = buf.st_size;
    actualSize += sizeof(File);
    attrs.mtime = (uint64_t)buf.st_mtim.tv_sec;
    attrs.userId = buf.st_uid;
    attrs.groupId = buf.st_gid;
    attrs.mode = buf.st_mode;
}

void File::computeCRC()
{
    ifstream f{path, ios_base::binary};
    if (f.is_open())
    {
        f.seekg(0, std::ios::end);
        std::streamsize size = f.tellg();
        f.seekg(0, std::ios::beg);
        char* data = new char[size];
        f.read(data, size);
        f.close();

        crc32 = ::crc32(data, size);

        delete data;
    }
}

vector<char> File::serialize() const
{
    vector<char> data;

    serializeAppend(data, path);
    serializeAppend(data, rawSize);
    serializeAppend(data, actualSize);
    serializeAppend(data, crc32);
    serializeAppend(data, attrs.mtime);
    serializeAppend(data, attrs.userId);
    serializeAppend(data, attrs.groupId);
    serializeAppend(data, attrs.mode);

    return data;
}

void File::deserialize(const std::vector<char>& data)
{
    auto it = begin(data);
    path = deserializeConsume<decltype(path)>(it);
    rawSize = deserializeConsume<decltype(rawSize)>(it);
    actualSize = deserializeConsume<decltype(actualSize)>(it);
    crc32 = deserializeConsume<decltype(crc32)>(it);
    attrs.mtime = deserializeConsume<decltype(attrs.mtime)>(it);
    attrs.userId = deserializeConsume<decltype(attrs.userId)>(it);
    attrs.groupId = deserializeConsume<decltype(attrs.groupId)>(it);
    attrs.mode = deserializeConsume<decltype(attrs.mode)>(it);
}

std::vector<char> File::readAll() const
{
    string fullpath = parent->getPath()+"/"+path;
    int fd = open(fullpath.c_str(), O_RDWR);
    if (fd < 0)
        throw runtime_error("File::readAll: Unabled to open "+fullpath);

    size_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    vector<char> data(size);
    auto r = read(fd, data.data(), size);
    if (r<0)
        data.clear();

    close(fd);
    return data;
}
