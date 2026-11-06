#ifndef XMLRPC_RIG_H
#define XMLRPC_RIG_H

#include <string>

extern bool connected_to_client;

extern std::string xml_cat_string( std::string send );
extern void xml_cmd_string( std::string send );
extern std::string client_get_xcvr();
extern bool client_connection();
extern bool connect_to_client();

#endif
