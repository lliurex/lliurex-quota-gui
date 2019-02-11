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

class N4D {
public:
    string toString(xmlrpc_c::value item,bool exporting);
    bool validate_user(string authUser, string authPwd);
    bool validate_user(string n4dHost, string authUser, string authPwd);
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

#endif // N4D_H
