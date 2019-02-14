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
    vector<string> field_names;
    map<string,string> fields;
    UserData();

    string toString();
    bool operator!=(UserData& udata);
    bool operator==(UserData& udata);

    string getField(string fieldname);
    void setField(string fieldname,string value);
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
    UserData getUser(string name);
};

#endif // DATAMODEL_H
