/*
* g++ -Wunused -std=c++1z -o n4d_cli n4d_cli.cpp -lxmlrpc++ -lxmlrpc -lxmlrpc_xmlparse -lxmlrpc_xmltok -lxmlrpc_util -lxmlrpc_client++
*/

#include <cstdlib>
#include <string>
#include <algorithm>
#include <iostream>
#include <ctime>
#include <xmlrpc-c/girerr.hpp>
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/client_simple.hpp>
#include "n4d.h"

#include <QString>
using namespace std;

N4D::N4D(){
}
N4D::~N4D(){
}


string N4D::toString(xmlrpc_c::value item,bool exporting=false){
    string c_string = "";

    switch(item.type()){
        case xmlrpc_c::value::TYPE_STRING:{
            xmlrpc_c::value_string xml_string = xmlrpc_c::value_string(item);
            //c_string = static_cast<string>(xml_string);
            if (exporting){
                c_string = xml_string.crlfValue();
                c_string = "string/" + c_string;
            }else{
                c_string = xml_string.crlfValue();
                c_string = "'" + c_string + "'";
            }
            break;
        }
        case xmlrpc_c::value::TYPE_INT:{
            xmlrpc_c::value_int xml_int = xmlrpc_c::value_int(item);
            //int i = static_cast<int>(xml_int);
            int i = xml_int.cvalue();
            if (exporting){
                c_string = "int/" + to_string(i);
            }else{
                c_string = to_string(i);
            }
            break;
        }
        case xmlrpc_c::value::TYPE_I8:{
            xmlrpc_c::value_i8 xml_long = xmlrpc_c::value_i8(item);
            //long long i = static_cast<long long>(xml_long);
            long long i = xml_long.cvalue();
            if (exporting){
                c_string = "long/" + to_string(i);
            }else{
                c_string = to_string(i);
            }
            break;
        }
        case xmlrpc_c::value::TYPE_DOUBLE:{
            xmlrpc_c::value_double xml_double = xmlrpc_c::value_double(item);
            //double i = static_cast<double>(xml_double);
            double i = xml_double.cvalue();
            if (exporting){
                c_string = "double/" + to_string(i);
            }else{
                c_string = to_string(i);
            }
            break;
        }
        case xmlrpc_c::value::TYPE_BYTESTRING:{
            xmlrpc_c::value_bytestring xml_bytestr = xmlrpc_c::value_bytestring(item);
            //vector<unsigned char> vchar(xml_bytestr.vectorUcharValue());
            vector<unsigned char> vchar = xml_bytestr.cvalue();
            c_string = string(vchar.begin(),vchar.end());
            break;
        }
        case xmlrpc_c::value::TYPE_BOOLEAN:{
            xmlrpc_c::value_boolean xml_bool = xmlrpc_c::value_boolean(item);
            //bool i = static_cast<bool>(xml_bool);
            bool i = xml_bool.cvalue();
            if (exporting){
                if (i){
                    c_string = "bool/true";
                }else{
                    c_string = "bool/false";
                }
            }else{
                if (i){
                    c_string = "True";
                }else{
                    c_string = "False";
                }
            }
            break;
        }
        case xmlrpc_c::value::TYPE_NIL:{
            c_string = "None";
            break;
        }
        case xmlrpc_c::value::TYPE_DATETIME:{
            xmlrpc_c::value_datetime xml_dt = xmlrpc_c::value_datetime(item);
            time_t t = xml_dt.cvalue();
            tm * ptm = localtime(&t);
            char buf[32];
            strftime(buf,32,"%a, %d.%m.%Y %H:%M:%S",ptm);
            c_string = string(buf);
            break;
        }
        case xmlrpc_c::value::TYPE_ARRAY:{
            xmlrpc_c::value_array xml_array = xmlrpc_c::value_array(item);
            vector<xmlrpc_c::value> c_vector = xml_array.vectorValueValue();
            c_string = "[";
            if (exporting){
                c_string = "array/" + c_string;
            }
            if (c_vector.size() != 0){
                for (auto const& i : c_vector){
                    c_string += toString(i,exporting) + ",";
                }
                c_string.pop_back();
            }
            c_string += "]";
            break;
        }
        case xmlrpc_c::value::TYPE_STRUCT:{
            xmlrpc_c::value_struct xml_struct = xmlrpc_c::value_struct(item);
            //map<string,xmlrpc_c::value> c_map = static_cast<map<string,xmlrpc_c::value>>(xml_struct);
            map<string,xmlrpc_c::value> c_map = xml_struct.cvalue();
            c_string = "{";
            if (exporting){
                c_string = "struct/" +c_string;
            }
            if (c_map.size() != 0){
                for (auto const& [key,val] : c_map){
                    if (exporting){
                        c_string += "string/" + key + ":" + toString(val,exporting) + ",";
                    }else{
                        c_string += "'" + key + "':" + toString(val,exporting) + ",";
                    }
                }
                c_string.pop_back();
            }
            c_string += "}";
            break;
        }
        default:{
            cout << "ERROR UNKNOWN TYPE" << endl;
            exit(1);
        }
    }
    return c_string;
}

xmlrpc_c::value N4D::parse_simple_param(string param){
    int pfxpos = param.find_first_of('/');
    string prefix = param.substr(0,pfxpos);
    std::transform(prefix.begin(),prefix.end(),prefix.begin(),::tolower);
    string value = param.substr(pfxpos+1);
    xmlrpc_c::value r;

    // int, string, long, double, bool, datetime, array, struct
    bool done = false;
    if (prefix == "string"){
        xmlrpc_c::value_string str(value);
        r = str;
        done = true;
    }
    if (prefix == "int"){
        int n = atoi(value.data());
        xmlrpc_c::value_int i(n);
        r = i;
        done = true;
    }
    if (prefix == "long"){
        char *pend;
        long long l = strtoll(value.data(),&pend,10);
        xmlrpc_c::value_i8 xmlval(l);
        r = xmlval;
        done = true;
    }
    if (prefix == "double"){
        double d = atof(value.data());
        xmlrpc_c::value_double xmlval(d);
        r = xmlval;
        done = true;
    }
    if (prefix == "bool"){
        bool b;
        transform(value.begin(),value.end(),value.begin(),::tolower);
        if (value == "true"){
            b = true;
        }
        if (value == "false"){
            b = false;
        }
        xmlrpc_c::value_boolean xmlval(b);
        r = xmlval;
        done = true;
    }
    if (prefix == "datetime"){
        time_t n = atoi(value.data());
        xmlrpc_c::value_datetime xmltime(n);
        r = xmltime;
        done = true;
    }
    if (!done){
        cout << "Error parsing simple param: " << param << endl;
    }
    return r;
}

xmlrpc_c::value N4D::parse_param(string param){
    int pfxpos = param.find_first_of('/');
    string prefix = param.substr(0,pfxpos);
    std::transform(prefix.begin(),prefix.end(),prefix.begin(),::tolower);
    string value = param.substr(pfxpos+1);
    xmlrpc_c::value v;

    if (prefix == "array"){
        v = parse_array(value);
    }else if (prefix == "struct"){
        v = parse_struct(value);
    }else{
        v = parse_simple_param(param);
    }
    return v;
}

// {}
// {string/4:int/2}
// {string/4:int/2,string/hola que tal:string/jeje}

xmlrpc_c::value_struct N4D::parse_struct(string param){
    size_t ini,mid,mid2,end;
    ini = param.find('{');
    mid = param.find(',');
    end = param.find('}');
    string tok,key,value;
    map<string,xmlrpc_c::value> structData;

    while(ini+1 < end){
        if (mid == string::npos){
            tok = param.substr(ini+1,end-ini-1);
            mid2 = tok.find(':');
            key = tok.substr(0,mid2);
            value = tok.substr(mid2+1,tok.size()-mid2);
            ini = end;
        }else{
            tok = param.substr(ini+1,mid-ini-1);
            mid2 = tok.find(':');
            key = tok.substr(0,mid2);
            value = tok.substr(mid2+1,tok.size()-mid2);
            ini = mid;
            mid = param.find(',',ini+1);
            mid2 = param.find(':',mid2+1);
        }
        xmlrpc_c::value_string xmlstr = parse_simple_param(key);
        xmlrpc_c::value xmlval = parse_param(value);
        pair<string,xmlrpc_c::value> elem = pair<string,xmlrpc_c::value>(xmlstr.crlfValue(),xmlval);
        structData.insert(elem);
    }
    return xmlrpc_c::value_struct(structData);
}

// []
// [string/4]
// [string/4,int/2,string/hola que tal/string/jeje]

xmlrpc_c::value_array N4D::parse_array(string param){
    size_t ini,mid,end;
    ini = param.find('[');
    mid = param.find(',');
    end = param.find(']');
    string tok;
    vector<xmlrpc_c::value> v;

    while(ini+1 < end){
        if (mid == string::npos){
            tok = param.substr(ini+1,end-ini-1);
            ini = end;
        }else{
            tok = param.substr(ini+1,mid-ini-1);
            ini = mid;
            mid = param.find(',',ini+1);
        }
        v.push_back(parse_param(tok));
    }
    return xmlrpc_c::value_array(v);
}

string N4D::clean_extra_spaces(string s){
    string r,tok;
    bool allow = false;
    int j=0;
    for (unsigned int i=0;i<s.size();i++){
        if ( i < 6 ){
            r = r +s[i];
            j++;
        }else{
            tok = r.substr(j-6,6);
            if (tok == "string"){
                allow = true;
            }
            if (allow){
                switch(s[i]){
                    case ']':
                    case ',':
                    case '}':
                        allow = false;
                        [[fallthrough]];
                    default:
                        r = r + s[i];
                        j++;
                        break;
                }
            }else{
                if (s[i] != ' '){
                    r = r + s[i];
                    j++;
                }
            }
        }
    }
    return r;
}

void N4D::process_params(xmlrpc_c::paramList &callParams, vector<string> params){
    xmlrpc_c::value v;
    for (unsigned int i=0; i<params.size(); i++){
        string p = clean_extra_spaces(params[i]);
        v = parse_param(p);
        callParams.add(v);
    }
}

string N4D::validate_user(string authUser, string authPwd){
    return make_call(authUser,authPwd,"","validate_user",true);
}

string N4D::validate_user(string n4dHost,string authUser, string authPwd){
    return make_call(n4dHost,authUser,authPwd,"","validate_user",true);
}

string N4D::make_anon_call(string className="", string methodName="", bool auth_as_param=false){
    return make_call("","","",className,methodName,vector<string>(),auth_as_param);
}

string N4D::make_anon_call(string n4dHost="", string className="", string methodName="", bool auth_as_param=false){
    return make_call(n4dHost,"","",className,methodName,vector<string>(),auth_as_param);
}

string N4D::make_anon_call(string className="", string methodName="", vector<string> params=vector<string>(), bool auth_as_param=false){
    return make_call("","","",className,methodName,params,auth_as_param);
}

string N4D::make_call(string authUser="", string authPwd="", string className="", string methodName="", bool auth_as_param=false){
    return make_call("",authUser,authPwd,className,methodName,vector<string>(),auth_as_param);
}

string N4D::make_call(string n4dHost="", string authUser="", string authPwd="", string className="", string methodName="", bool auth_as_param=false){
    return make_call(n4dHost,authUser,authPwd,className,methodName,vector<string>(),auth_as_param);
}

string N4D::make_call(string authUser="", string authPwd="", string className="", string methodName="", vector<string> params=vector<string>(), bool auth_as_param=false){
    return make_call("",authUser,authPwd,className,methodName,params,auth_as_param);
}

string N4D::make_call(string n4dHost="", string authUser="", string authPwd="", string className="", string methodName="", vector<string> params=vector<string>(), bool auth_as_param=false){
    // cout << "Using " << n4dHost << endl;
    // Parametized options that allow n4dclient work with self-signed certificates
    xmlrpc_c::clientXmlTransport_curl myTransport
      (xmlrpc_c::clientXmlTransport_curl::constrOpt()
       .no_ssl_verifyhost(true)
       .no_ssl_verifypeer(true)
       );

    // Custom client with parametized options
    xmlrpc_c::client_xml myClient(&myTransport);

    // Example: Simple client that don't support self-signed certificates
        // xmlrpc_c::clientSimple myClient;

    xmlrpc_c::paramList callParams;
    // ((user,password)| ) classname? Parameters*

    if (auth_as_param){
        callParams.add(xmlrpc_c::value_string(authUser));
        callParams.add(xmlrpc_c::value_string(authPwd));
    }else{
        // If need authentication
        if (authUser == "" or authPwd == ""){
            callParams.add(xmlrpc_c::value_string(""));
        }else{
            vector<xmlrpc_c::value> vParams;
            vParams.push_back(xmlrpc_c::value_string(authUser));
            vParams.push_back(xmlrpc_c::value_string(authPwd));
            xmlrpc_c::value_array aParams(vParams);
            callParams.add(aParams);
        }
        if (className != ""){
            callParams.add(xmlrpc_c::value_string(className));
        }
    }
    if (params.size() != 0){
        process_params(callParams,params);
    }
    xmlrpc_c::rpcPtr myRpcP(methodName, callParams);

    if (n4dHost == ""){
        n4dHost = "https://localhost:9779/RPC2";
    }
    xmlrpc_c::carriageParm_curl0 myCarriageParm(n4dHost);

    myRpcP->call(&myClient,&myCarriageParm);

    xmlrpc_c::value returned = myRpcP->getResult();

    // Example return if a simple call is used
    // string const res(xmlrpc_c::value_array(myRpcP->getResult()));

    string ret = toString(returned,true);
    return ret;

    // Example simple call with two params
    //myClient.call(serverUrl, methodName, "ii", &result, 5, 7);

    //string const res = xmlrpc_c::value_string(result);
}

// value: any_struct->any_field=one_value, any_array[]=any_struct->any_field=one_value, any_type=any_value


QtN4DWorker::QtN4DWorker(){
    qRegisterMetaType<QtN4DWorker::Methods>("Methods");
    n4d = new N4D();
}

QtN4DWorker::~QtN4DWorker(){
    delete n4d;
}

void QtN4DWorker::set_auth(QString user_param, QString pwd_param){
    user = user_param;
    pwd = pwd_param;
}

void QtN4DWorker::validate_user(){
    string res = n4d->validate_user(user.toStdString(),pwd.toStdString());
    emit n4d_call_completed(Methods::LOGIN,QString(res.data()));
}

void QtN4DWorker::get_table_data(){
    string res = n4d->make_call(user.toStdString(),pwd.toStdString(),"QuotaManager","get_quotas",false);
    emit n4d_call_completed(Methods::GET_DATA,QString(res.data()));
}

void QtN4DWorker::get_system_status(){
    string res = n4d->make_call(user.toStdString(),pwd.toStdString(),"QuotaManager","get_local_status",false);
    emit n4d_call_completed(Methods::GET_STATUS,QString(res.data()));
}

void QtN4DWorker::get_configured_status(){
    string res = n4d->make_call(user.toStdString(),pwd.toStdString(),"QuotaManager","get_quotafile",false);
    emit n4d_call_completed(Methods::GET_CONFIGURED,QString(res.data()));
}

void QtN4DWorker::get_golem_groups(){
    string res = n4d->make_call(user.toStdString(),pwd.toStdString(),"Golem","get_available_groups",false);
    emit n4d_call_completed(Methods::GET_GOLEM_GROUPS,QString(res.data()));
}

string n4dresult2json(string result){
    string buffer;
    string output;

    unsigned int i = 0;

    while(i < result.length()){
        switch(result[i]){
        case '/':{
            buffer.clear();
            break;
        }
        case ',':
        case ']':
        case '[':
        case '{':
        case '}':
        case ':':
        {
            if (buffer != ""){
                output += '"' + buffer + '"' + result[i];
            }else{
                output += result[i];
            }
            buffer.clear();
            break;
        }
        default:{
            buffer += result[i];
            break;
        }
        }
        i++;
    }
    return output;
}

bool n4dvalidator(n4dleaf* result, n4dleaf* query){
    if (result->type != query->type){
        return false;
    }
    if (result->value == query->value){
        switch(query->parent->type){
        case n4dtypetree::STRUCT_VALUE:{
            list<n4dtree*>::iterator first_child = result->parent->parent->childs_tree.begin();
            list<n4dtree*>::iterator query_first_child = query->parent->parent->childs_tree.begin();
            n4dtree* struct_key_item = (*first_child);
            list<n4dleaf*>::iterator leaf = struct_key_item->childs_leaf.begin();
            n4dtree* query_struct_key_item = (*query_first_child);
            list<n4dleaf*>::iterator query_leaf = query_struct_key_item->childs_leaf.begin();
            // DO NOT USE RECURSIVE SOLUTION !! CYCLE !!
            if ((*query_leaf)->type != (*leaf)->type){
                return false;
            }
            if ((*query_leaf)->value != (*leaf)->value){
                return false;
            }
            return true;
            break;
        }
        case n4dtypetree::STRUCT_KEY:{
            list<n4dtree*>::iterator last_child = result->parent->parent->childs_tree.begin();
            list<n4dtree*>::iterator query_last_child = query->parent->parent->childs_tree.begin();
            last_child++;
            query_last_child++;
            n4dtree* struct_value_item = (*last_child);
            n4dtree* query_struct_value_item = (*query_last_child);

            return n4dvalidator(struct_value_item,query_struct_value_item);
            break;
        }
        default:
            return true;
            break;
        }
    }
    return false;
}

/* Avoid -Wunused warnings */
#define UNUSED(x) (void)(x)

bool n4dvalidator(n4dtree* result, n4dleaf* query){
    UNUSED(result);
    UNUSED(query);
    return false;
}

bool n4dvalidator(n4dleaf* result, n4dtree* query){
    UNUSED(result);
    UNUSED(query);
    return false;
}

bool n4dvalidator(n4dtree* result, n4dtree* query){
    n4dtree* presult;
    n4dtree* pquery;
    presult = result;
    pquery = query;
    bool found = false;
    bool notfound = false;
    while (!found && !notfound){
        if (pquery->type != presult->type){
            return false;
        }
        for (auto const& i: pquery->childs_leaf){
            if (presult->childs_leaf.size() < pquery->childs_leaf.size() ){
                return false;
            }
            for (auto const& j: presult->childs_leaf){
                if (n4dvalidator(j,i)){
                    found = true;
                    break;
                }
            }
            if (found){
                break;
            }
        }
        if (found){
            break;
        }
        for (auto const& i: pquery->childs_tree){
            if (presult->childs_tree.size() < pquery->childs_tree.size() ){
                return false;
            }
            for (auto const& j: presult->childs_tree){
                if (n4dvalidator(j,i)){
                    found = true;
                    break;
                }
            }
            if (found){
                break;
            }
        }
        if (found){
            break;
        }
        notfound = true;
    }

    if (found){
        return true;
    }else{
        return false;
    }
}

bool n4dvalidator(n4dtree* result,string query){
    return n4dvalidator(result, n4dtokenparser(query));
}

bool n4dvalidator(string result, n4dtree* query){
    return n4dvalidator(n4dtokenparser(result),query);
}

bool n4dvalidator(string result, string query){
    return n4dvalidator(n4dtokenparser(result),n4dtokenparser(query));
}

n4dtree* n4dtokenparser(string str){
    return n4dtokenparser(n4dtokenizer(str));
}

n4dtree* n4dtokenparser(list<n4dtoken*> list){
    n4dtree* tree = new n4dtree();
    tree->type=n4dtypetree::ROOT;
    tree->parent=tree;
    n4dtokenparser(list, tree);
    return tree;
}

void n4dtokenparser(list<n4dtoken*> l, n4dtree* parent){
    unsigned int i_array=0;
    unsigned int i_struct=0;

#ifdef _N4D_DEBUG_
    cout << "Building tree for:" << endl;
    for (auto const& i: l){
        cout << i->value;
    }
    cout << endl ;
#endif
    std::list<n4dtoken*>::iterator it;
    for (it=l.begin();it!=l.end();it++){
        n4dtree* tree;
        switch ((*it)->type) {
        case n4dtypetokens::START_ARRAY:{
            if (parent->type == n4dtypetree::TREE_ARRAY){
                tree = new n4dtree();
                tree->parent = parent;
                tree->type = n4dtypetree::ARRAY_ITEM;
                parent->childs_tree.push_back(tree);
                parent = tree;
            }
            tree = new n4dtree();
            tree->parent = parent;
            tree->type = n4dtypetree::TREE_ARRAY;
            parent->childs_tree.push_back(tree);
            i_array++;
            list<n4dtoken*> sublist;
            list<n4dtoken*>::iterator end = it;
            while (i_array != 0 && end != l.end()){
                end++;
                if( (*end)->type == n4dtypetokens::START_ARRAY ){
                    i_array++;
                }
                if ( (*end)->type == n4dtypetokens::END_ARRAY ){
                    i_array--;
                }
            }
            if (end == l.end() and i_array != 0){
                cerr << "Error building tree" << endl;
                exit(1);
            }
            sublist.splice(sublist.begin(),l,std::next(it),end);
            list<n4dtoken*>::iterator j = l.begin();
            end++;
            while (j != end){
                delete (*j);
                j++;
            }
            l.erase(l.begin(),end);
            it = end;
#ifdef _N4D_DEBUG_
            cout << "Detected array with items: " << endl;
            for (auto const& j2: sublist){
                cout << j2->value;
            }
            cout << endl;
#endif
            list<n4dtoken*>::iterator i = sublist.begin();
            while(i != sublist.end()){
                int s_level = 0;
                int a_level = 0;
                int level = 0;
                end = sublist.begin();
                while (end != sublist.end() && (((*end)->type != n4dtypetokens::NEXT_ITEM) || (level != 0))){
                    if ((*end)->type != n4dtypetokens::START_ARRAY){
                        a_level++;
                    }
                    if ((*end)->type != n4dtypetokens::END_ARRAY){
                        a_level--;
                    }
                    if ((*end)->type != n4dtypetokens::START_STRUCT){
                        s_level++;
                    }
                    if ((*end)->type != n4dtypetokens::END_STRUCT){
                        s_level--;
                    }
                    level = s_level + a_level;
                    end++;
                }
                if (end == sublist.end()){
#ifdef _N4D_DEBUG_
                    cout << "Value for array item: " << endl;
                    for (auto const& j: sublist){
                        cout << j->value;
                    }
                    cout << endl << "END OF ARRAY ITEMS PARSING" << endl;;
#endif
                    n4dtokenparser(sublist,tree);
                    i = end;
                }else{
                    list<n4dtoken*> sublist2;
                    sublist2.splice(sublist2.begin(),sublist,sublist.begin(),end);
#ifdef _N4D_DEBUG_
                    cout << "Value for array item (more to come next):" << endl;
                    for (auto const& j: sublist2){
                        cout << j->value;
                    }
                    cout << endl;
#endif
                    i = std::next(end);
                    delete (*end);
                    sublist.erase(end);
                    n4dtokenparser(sublist2,tree);
                }
            }

            break;
        }
        case n4dtypetokens::START_STRUCT:{
            if (parent->type == n4dtypetree::TREE_ARRAY){
                tree = new n4dtree();
                tree->parent = parent;
                tree->type = n4dtypetree::ARRAY_ITEM;
                parent->childs_tree.push_back(tree);
                parent = tree;
            }
            tree = new n4dtree();
            tree->parent = parent;
            tree->type = n4dtypetree::TREE_STRUCT;
            parent->childs_tree.push_back(tree);
            i_struct++;
            list<n4dtoken*> sublist;
            list<n4dtoken*>::iterator end = it;
            while (i_struct != 0 && end != l.end()){
                end++;
                if( (*end)->type == n4dtypetokens::START_STRUCT ){
                    i_struct++;
                }
                if ( (*end)->type == n4dtypetokens::END_STRUCT ){
                    i_struct--;
                }
            }
            if (end == l.end() and i_struct != 0){
                cerr << "Error building tree" << endl;
                exit(1);
            }
            sublist.splice(sublist.begin(),l,std::next(it),end);
#ifdef _N4D_DEBUG_
            cout << "Detected struct with items:" << endl;
            for (auto const& j: sublist){
                cout << j->value;
            }
            cout << endl;
#endif
            end++;
            for (list<n4dtoken*>::iterator i=l.begin();i!=end;i++){
                delete (*i);
            }
            l.erase(l.begin(),end);
            it=end;
            list<n4dtoken*>::iterator i = sublist.begin();
            while(i != sublist.end()){

                n4dtree* itemtree = new n4dtree();
                itemtree->parent = tree;
                itemtree->type = n4dtypetree::STRUCT_ITEM;
                tree->childs_tree.push_back(itemtree);

                n4dtree* keytree = new n4dtree();
                keytree->parent = itemtree;
                keytree->type = n4dtypetree::STRUCT_KEY;
                itemtree->childs_tree.push_back(keytree);

                n4dtree* valuetree = new n4dtree();
                valuetree->parent = itemtree;
                valuetree->type = n4dtypetree::STRUCT_VALUE;
                itemtree->childs_tree.push_back(valuetree);

                n4dleaf* keyleaf = new n4dleaf();
                keyleaf->parent = keytree;
                keyleaf->type = (*i)->type;
#ifdef _N4D_DEBUG_
                cout << "Detected struct item keyleaf: " << endl << "keytype=" << (*i)->value ;
#endif
                keytree->childs_leaf.push_back(keyleaf);
                delete(*i);
                i++;
                delete(*i);
                i++;
                keyleaf->value =  (*i)->value;
#ifdef _N4D_DEBUG_
                cout << " keyvalue=" << (*i)->value << endl;
#endif
                delete(*i);
                i++;
                delete(*i);
                i++;
                sublist.erase(sublist.begin(),i);
                i = sublist.begin();

                end = i;
                int s_level = 0;
                int a_level = 0;
                int level = 0;
                while (end != sublist.end() && (((*end)->type != n4dtypetokens::NEXT_ITEM) || (level != 0))){
                    if ((*end)->type != n4dtypetokens::START_ARRAY){
                        a_level++;
                    }
                    if ((*end)->type != n4dtypetokens::END_ARRAY){
                        a_level--;
                    }
                    if ((*end)->type != n4dtypetokens::START_STRUCT){
                        s_level++;
                    }
                    if ((*end)->type != n4dtypetokens::END_STRUCT){
                        s_level--;
                    }
                    level = s_level + a_level;
                    end++;
                }
                if (end == sublist.end()){
#ifdef _N4D_DEBUG_
                    cout << "Value for struct item: " << endl;
                    for (auto const& j: sublist){
                        cout << j->value;
                    }
                    cout << endl << "END OF STRUCT ITEMS PARSING" << endl;;
#endif
                    n4dtokenparser(sublist,valuetree);
                    i = end;
                }else{
                    list<n4dtoken*> sublist2;
                    sublist2.splice(sublist2.begin(),sublist,sublist.begin(),end);
#ifdef _N4D_DEBUG_
                    cout << "Value for struct item (more to come next):" << endl;
                    for (auto const& j: sublist2){
                        cout << j->value;
                    }
                    cout << endl;
#endif
                    i = std::next(end);
                    delete (*end);
                    n4dtokenparser(sublist2,valuetree);
                }
            }
            break;
        }
        case n4dtypetokens::TYPE_STRING:
        case n4dtypetokens::TYPE_BOOL:
        case n4dtypetokens::TYPE_INT:{
            if (parent->type == n4dtypetree::TREE_ARRAY){
                tree = new n4dtree();
                tree->parent = parent;
                tree->type = n4dtypetree::ARRAY_ITEM;
                parent->childs_tree.push_back(tree);
                parent = tree;
            }
            tree = new n4dtree();
            tree->parent = parent;
            tree->type = n4dtypetree::TREE_ITEM;
            n4dleaf* leaf = new n4dleaf();
            leaf->parent = parent;
            leaf->type = (*it)->type;
#ifdef _N4D_DEBUG_
            cout << "Detected leaf :" << endl << (*it)->value;
#endif
            delete (*it);
            it++;
#ifdef _N4D_DEBUG_
            cout << (*it)->value;
#endif
            delete (*it);
            it++;
#ifdef _N4D_DEBUG_
            cout << (*it)->value << endl;
#endif
            leaf->value = (*it)->value;
            delete (*it);
            tree->childs_leaf.push_back(leaf);
            parent->childs_tree.push_back(tree);
            ;
            break;
        }
        default:
            break;
        }

    }

}

list<n4dtoken*> n4dtokenizer(string str){
    string buffer;
    list<n4dtoken*> tokens;

    unsigned int i = 0;
    while (i< str.size()){
        switch(str[i]){
        case '{':{
            n4dtoken* tok = new n4dtoken();
            tok->value='{';
            tok->type=n4dtypetokens::START_STRUCT;
            tokens.push_back(tok);
            break;
        }
        case '}':{
            n4dtoken* tok = new n4dtoken();
            tok->value='}';
            tok->type=n4dtypetokens::END_STRUCT;
            if (buffer != ""){
                n4dtoken* tok2 = new n4dtoken();
                tok2->value=buffer;
                tok2->type=n4dtypetokens::ANY;
                tokens.push_back(tok2);
                buffer = "";
            }
            tokens.push_back(tok);
            break;
        }
        case '[':{
            n4dtoken* tok = new n4dtoken();
            tok->value='[';
            tok->type=n4dtypetokens::START_ARRAY;
            tokens.push_back(tok);
            break;
        }
        case ']':{
            n4dtoken* tok = new n4dtoken();
            tok->value=']';
            tok->type=n4dtypetokens::END_ARRAY;
            if (buffer != ""){
                n4dtoken* tok2 = new n4dtoken();
                tok2->value = buffer;
                tok2->type=n4dtypetokens::ANY;
                tokens.push_back(tok2);
                buffer = "";
            }
            tokens.push_back(tok);
            break;
        }
        case ',':{
            n4dtoken* tok = new n4dtoken();
            tok->value=',';
            tok->type=n4dtypetokens::NEXT_ITEM;
            if (buffer != ""){
                n4dtoken* tok2 = new n4dtoken();
                tok2->value=buffer;
                tok2->type=n4dtypetokens::ANY;
                tokens.push_back(tok2);
                buffer = "";
            }
            tokens.push_back(tok);
            break;
        }
        case ':':{
            n4dtoken* tok = new n4dtoken();
            tok->value=':';
            tok->type=n4dtypetokens::STRUCT_SEPARATOR;
            if (buffer != ""){
                n4dtoken* tok2 = new n4dtoken();
                tok2->value=buffer;
                tok2->type=n4dtypetokens::STRUCT_KEY;
                tokens.push_back(tok2);
                buffer = "";
            }
            tokens.push_back(tok);
            break;
        }
        case '/':{
            n4dtoken* tok = new n4dtoken();
            tok->value='/';
            tok->type=n4dtypetokens::TYPE_SEPARATOR;
            n4dtoken *tok2 = new n4dtoken();
            tok2->value = buffer;
            if (buffer == "string"){
                tok2->type = n4dtypetokens::TYPE_STRING;
            }
            if (buffer == "int"){
                tok2->type = n4dtypetokens::TYPE_STRING;
            }
            if (buffer == "bool"){
                tok2->type = n4dtypetokens::TYPE_BOOL;
            }
            if (buffer == "struct"){
                tok2->type = n4dtypetokens::TYPE_STRUCT;
            }
            if (buffer == "array"){
                tok2->type = n4dtypetokens::TYPE_ARRAY;
            }
            tokens.push_back(tok2);
            tokens.push_back(tok);
            buffer = "";
            break;
        }
        case '=':{
            n4dtoken* tok = new n4dtoken();
            tok->value="=";
            tok->type=n4dtypetokens::OP_EQUAL;
            tokens.push_back(tok);
            buffer = "";
            break;
        }
        default:{
            buffer += str[i];
            break;
        }
        }

        i++;
    }
    if (buffer != ""){
        n4dtoken* tok = new n4dtoken();
        tok->value=buffer;
        tok->type=n4dtypetokens::ANY;
        tokens.push_back(tok);
    }
    //testing for debug
    string returned="";
    for(auto const& i: tokens){
        returned += i->value;
    }
    if (str != returned){
        //cout << str << endl;
        //cout << returned << endl;
        cerr << "ERROR TOKENIZING !!";
    }
    return tokens;
}

//    try {
//    } catch (exception const& e) {
//        cerr << "Client threw error: " << e.what() << endl;
//    } catch (...) {
//        cerr << "Client threw unexpected error." << endl;
//    }


