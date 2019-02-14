#include "datamodel.h"

DataModel::DataModel()
{
}
DataModel::~DataModel()
{
}
string DataModel::toString(){
    return userlist.toString();
}
void DataModel::LoadUsersFromString(string str){
    str_userdata = str;

    unsigned int i = 0;
    string buffer;
    string user;
    string key,value;
    map<string,string> attrs;
    bool initiated = false;
    bool init_user = false;
    bool go_to_value = false;
    bool go_to_key = false;
    bool end_value = false;
    bool end_user = false;
    UserData userdata;

    while (i < str_userdata.size()){
        switch(str_userdata[i]){
        case '{':{
            if (! initiated){
                initiated = true;
            }else{
                init_user = true;
            }
            break;
        }
        case '}':{
            if (init_user){
                init_user = false;
                end_user = true;
            }else{
                if (initiated){
                    initiated = false;
                }
            }
            break;
        }
        case ':':{
            go_to_value = ! go_to_value;
            break;
        }
        case '/':{
            go_to_key = ! go_to_key;
            break;
        }
        case ',':{
            end_value = true;
            break;
        }
        default:{
            buffer += str_userdata[i];
            break;
        }
        };
        if (go_to_key){
            go_to_key = ! go_to_key;
            buffer = "";
        }
        if (!init_user){
            if (go_to_value){
                user = buffer;
                buffer = "";
                go_to_value = !go_to_value;
            }
        }else{
            if (go_to_value){
                key=buffer;
                buffer="";
                go_to_value = ! go_to_value;
            }
        }
        if(end_value){
            end_value = ! end_value;
            value = buffer;
            attrs.insert(pair<string,string>(key,value));
        }
        if (end_user){
            end_user = ! end_user;
            userlist.putUser(user,attrs);
            user = "";
            attrs.clear();
        }

        i++;
    }
}

UserData DataModel::getUser(string name){
    return userlist.getUser(name);
}

map<string,UserData> DataModel::getTableData(){
    return userlist.getMap();
}

map<string,UserData> DataModelUser::getMap(){
    return list;
}

void DataModelUser::putUser(string name, map<string,string> userdata){
    UserData data;
    bool error = false;

    // todo: check duplicates
    for (auto const& elem: data.field_names){
        if (elem == "name"){
            continue;
        }
        if (userdata.find(elem) == userdata.end()){
            cerr << "Error: Incomplete user data for name " << name << ", need " + elem << endl;
            error = true;
        }else{
            data.fields[elem] = userdata[elem];
        }
    }
    if (name != ""){
        data.fields[string("name")]= name;
    }else{
        error = true;
    }
    if (!error){
        list.insert(pair<string,UserData>(data.fields[string("name")],data));
    }else{
        cerr << "Error adding user to model, skipping !" << endl;
    }
}
void DataModelUser::putUser(string name, UserData udata){
    list.insert(pair<string,UserData>(name,udata));
}

bool DataModelUser::UserExists(string name){
    if (list.find(name) == list.end()){
        return false;
    }
    return true;
}

UserData DataModelUser::getUser(string name){
    if (UserExists(name)){
        return list[string(name)];
    }
    return UserData();
}

string DataModelUser::toString(){
    string str="";
    for (auto const& [key,value]: list){
        string k = key;
        UserData data = value;
        str += data.toString() + "\n";
    }
    return str;
}

UserData::UserData(){
    field_names.push_back(string("name"));
    field_names.push_back(string("filegrace"));
    field_names.push_back(string("filehardlimit"));
    field_names.push_back(string("filesoftlimit"));
    field_names.push_back(string("filestatus"));
    field_names.push_back(string("fileused"));
    field_names.push_back(string("spacegrace"));
    field_names.push_back(string("spacehardlimit"));
    field_names.push_back(string("spacesoftlimit"));
    field_names.push_back(string("spacestatus"));
    field_names.push_back(string("spaceused"));

    for (auto const& name: field_names){
        fields.insert(pair<string,string>(name,""));
    }
}

bool UserData::operator!=(UserData& udata){
       return ! operator ==(udata);
}

bool UserData::operator==(UserData& udata){
    for(auto const& name: field_names){
        if (udata.getField(name) != fields[name] ){
            return false;
        }
    }
    return true;
}

string UserData::toString(){
    return "User: " + fields["name"] + " Attributes F(" + fields["fileused"] + "," + fields["filestatus"] + "," + fields["filesoftlimit"] + "," + fields["filehardlimit"] + "," + fields["filegrace"] + ") S(" + fields["spaceused"] + "," + fields["spacestatus"] + "," + fields["spacesoftlimit"] + "," + fields["spacehardlimit"] + "," + fields["spacegrace"] + ")";
}

string UserData::getField(string fieldname){
    for (string name: field_names){
        if (name==fieldname){
            return fields[name];
        }
    }
    return string();
}
void UserData::setField(string fieldname,string value){
    //todo check fieldname existence
    fields[fieldname]=value;
}
