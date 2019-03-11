#ifndef N4D_H
#define N4D_H

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

using namespace std;

#include <QObject>

class N4D {

public:
    N4D();
    ~N4D();
    string toString(xmlrpc_c::value item,bool exporting);
    string validate_user(string n4dHost, string authUser, string authPwd);
    string validate_user(string authUser, string authPwd);
    string make_anon_call(string className, string methodName, bool auth_as_param);
    string make_anon_call(string n4dHost, string className, string methodName, bool auth_as_param);
    string make_anon_call(string className, string methodName, vector<string> params, bool auth_as_param);
    string make_call(string authUser, string authPwd, string className, string methodName, bool auth_as_param);
    string make_call(string n4dHost, string authUser, string authPwd, string className, string methodName, bool auth_as_param);
    string make_call(string authUser, string authPwd, string className, string methodName, vector<string> params, bool auth_as_param);
    string make_call(string n4dHost, string authUser, string authPwd, string className, string methodName, vector<string> params, bool auth_as_param);

private:
    xmlrpc_c::value parse_simple_param(string param);
    xmlrpc_c::value parse_param(string param);
    xmlrpc_c::value_struct parse_struct(string param);
    xmlrpc_c::value_array parse_array(string param);
    string clean_extra_spaces(string s);
    void process_params(xmlrpc_c::paramList &callParams, vector<string> params);
};

class QtN4DWorker: public QObject{
    Q_OBJECT

public:
    enum class Methods { LOGIN, GET_DATA, GET_STATUS, GET_CONFIGURED, GET_GOLEM_GROUPS, DISABLE_SYSTEM, ENABLE_SYSTEM };
    Q_ENUM(Methods)

    QtN4DWorker(int nworker);
    ~QtN4DWorker();
    void set_auth(QString user_param, QString pwd_param);

signals:
    void n4d_call_completed(QtN4DWorker::Methods from, QString result, int serial);
    void finished_thread(int serial);

public slots:
    void validate_user();
    void get_table_data();
    void get_system_status();
    void get_configured_status();
    void get_golem_groups();
    void enable_system();
    void disable_system();

private:
    N4D* n4d;
    QString user,pwd;
    int serial;
};

enum class n4dtypetokens { START_ARRAY, END_ARRAY, START_STRUCT, END_STRUCT, STRUCT_KEY, STRUCT_SEPARATOR, NEXT_ITEM, TYPE_SEPARATOR, TYPE_STRING, TYPE_INT, TYPE_ARRAY, TYPE_STRUCT, TYPE_BOOL, OP_EQUAL, ANY};
enum class n4dtypetree {ROOT, TREE_ARRAY, TREE_STRUCT, TREE_ITEM, STRUCT_ITEM, STRUCT_KEY, STRUCT_VALUE, ARRAY_ITEM};

class n4dtoken {
public:
    n4dtypetokens type;
    string value;
};

class n4dtree;

class n4dleaf {
public:
    //n4dleaf();
    //~n4dleaf();
    n4dtree* parent;
    n4dtypetokens type;
    string value;
};

class n4dtree {
public:
    //n4dtree();
    //~n4dtree();
    n4dtypetree type;
    n4dtree* parent;
    list<n4dtree*> childs_tree;
    list<n4dleaf*> childs_leaf;
};

// TODO: make n4dresult2xml(string result);
// This make available to use XPath queries to get elements easily

string n4dresult2json(string result);
bool n4dvalidator(n4dleaf* result, n4dleaf* query);
bool n4dvalidator(n4dtree* result, n4dleaf* query);
bool n4dvalidator(n4dleaf* result, n4dtree* query);
bool n4dvalidator(n4dtree* result, n4dtree* query);
bool n4dvalidator(n4dtree* result, string query);
bool n4dvalidator(string result, n4dtree* query);
bool n4dvalidator(string result, string query);
n4dtree* n4dtokenparser(string str);
n4dtree* n4dtokenparser(list<n4dtoken*> list);
void n4dtokenparser(list<n4dtoken*> list, n4dtree* parent);
list<n4dtoken*> n4dtokenizer(string str);


#endif // N4D_H
