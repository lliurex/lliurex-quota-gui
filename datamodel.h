#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <string>
#include <cstdlib>
#include <map>
#include <iostream>
#include <vector>

using namespace std;

class UserData{
public:
    string field_names[11] = {  string("name"),
                                string("filegrace"),
                                string("filehardlimit"),
                                string("filesoftlimit"),
                                string("filestatus"),
                                string("fileused"),
                                string("spacegrace"),
                                string("spacehardlimit"),
                                string("spacesoftlimit"),
                                string("spacestatus"),
                                string("spaceused") };
    map<string,string> fields;
    UserData();
    string toString();
    string getField(string fieldname);
};

class DataModelUser{
    map<string,UserData> list;

public:
    void putUser(string name, map<string,string> userdata);
    void putUser(string name, UserData udata);
    bool UserExists(string name);
    UserData getUser(string name);
    string getUserAttribute(string name, string attribute);
    map<string,UserData> getMap();
    string toString();
};

class DataModel
{
    DataModelUser userlist;
    string str_userdata;
public:
    DataModel();
    ~DataModel();

    // todo: write iterators
    void LoadUsersFromString(string str);
    string toString();
    map<string,UserData> getTableData();
};

#endif // DATAMODEL_H
