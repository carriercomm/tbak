#include "folderdb.h"
#include "serialize.h"
#include <fstream>
#include <algorithm>
#include <iostream>

using namespace std;

FolderDB::FolderDB(const string &path)
{
    load(path);
}

FolderDB::FolderDB(const vector<char> &data)
{
    if (!data.empty())
        deserialize(data);
}

void FolderDB::save(const string &path) const
{
    vector<char> data = serialize();
    ofstream f(path, ios_base::binary | ios_base::trunc);
    if (f.is_open())
    {
        copy(begin(data), end(data), ostreambuf_iterator<char>(f));
        f.close();
    }
}

vector<char> FolderDB::serialize() const
{
    vector<char> data;

    vector<vector<char>> foldersData;
    for (const Folder& f : folders)
        foldersData.push_back(f.serialize());

    serializeAppend(data, foldersData);

    return data;
}

void FolderDB::load(const std::string& path)
{
    ifstream f(path, ios_base::binary);
    vector<char> data((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    if (f.is_open())
        f.close();

    if (!data.empty())
        deserialize(data);
}

void FolderDB::deserialize(const std::vector<char> &data)
{
    auto it = begin(data);

    vector<vector<char>> foldersData = deserializeConsume<decltype(foldersData)>(it);
    for (const vector<char>& vec : foldersData)
        folders.push_back(Folder(vec));
}

std::vector<Folder>& FolderDB::getFolders()
{
    return folders;
}

const std::vector<Folder>& FolderDB::getFolders() const
{
    return folders;
}

void FolderDB::addFolder(const Folder& folder)
{
    auto pred = [&folder](const Folder& f){return f.getPath() == folder.getPath();};
    if (find_if(begin(folders), end(folders), pred) != end(folders))
        return;

    folders.push_back(folder);
}

void FolderDB::addFolder(const std::string& path)
{
    auto pred = [&path](const Folder& f){return f.getPath() == path;};
    if (find_if(begin(folders), end(folders), pred) != end(folders))
        return;

    folders.push_back(Folder(path));
}

bool FolderDB::removeFolder(const std::string& path, bool archive)
{
    for (unsigned i=0; i<folders.size(); ++i)
    {
        if (folders[i].getPath() == path)
        {
            if (archive != (folders[i].getType() == FolderType::Archive))
                continue;
            folders[i].close();
            folders[i].removeData();
            folders.erase(begin(folders)+i);
            return true;
        }
    }
    return false;
}
