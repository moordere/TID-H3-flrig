// =====================================================================
//
// xmlrpc_rig.cxx
//
// connect to client xmlrpc server
//
// Copyright (C) 2023
//		Dave Freese, W1HKJ
//
// This is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// It is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// =====================================================================

#include <string>
#include <iostream>
#include <cmath>
#include <cstring>
#include <stdlib.h>

#include "XmlRpc.h"

#include "config.h"
#include "status.h"
#include "util.h"
#include "threads.h"
#include "tod_clock.h"

#include "xmlrpc_rig.h"
#include "trace.h"

using namespace XmlRpc;

int xmlrpc_verbosity = 0;

bool connected_to_client = false;

XmlRpcClient *flrig_client = (XmlRpcClient *)0;

void xmlrpc_ptt( int PTT)
{
	XmlRpcValue Args, result;
	Args = (int)PTT;

	try {
		flrig_client->execute("rig.set_ptt", Args, result);
	} catch (XmlRpcException *err) {
		xml_trace(2, "rig.set_ptt: %s", err->getMessage().c_str());
	}
}

bool xml_ptt_state()
{
	XmlRpcValue Args, result;

	try {
		flrig_client->execute("rig.get_ptt", Args, result);
		bool ptt = (int)result;
		return ptt;
	} catch (XmlRpcException *err) {
		xml_trace(2, "rig.get_ptt: %s", err->getMessage().c_str());
		return false;
	}
}

static std::string send_hex(std::string s)
{
	if (s.empty()) return "";
	std::string send  = "";
	char hexval[4] = "   ";
	for (size_t n = 0; n < s.length(); n++) {
		snprintf(hexval, sizeof(hexval), " %2X", (char)s[n]);
		send.append(hexval);
	}
	return send;
}

static std::string recv_hex(std::string s)
{
	if (s.empty()) return "";
	std::string recv = "";
	std::string hexval = "";
	unsigned int C;
	for (size_t n = 0; n < s.length(); n += 3) {
		hexval = s.substr(n, 3);
		sscanf(hexval.c_str(), " %2X", &C);
		recv += C & 0xFF;
	}
	return recv;
}

void xml_cmd_string(std::string send)
{
	XmlRpcValue Args, result;
		Args = send_hex(send);

	try {
		xml_trace(2, "rig_cmd_string send:", send.c_str());
		xml_trace(2, "                    ", ((std::string)(Args)).c_str());

		flrig_client->execute("rig.cmd_string", Args, result);

		return;

	} catch (XmlRpcException *err) {
		xml_trace(2, "rig.cmd_string: %s", err->getMessage().c_str());
		return;
	}
}

std::string xml_cat_string( std::string send )
{
	XmlRpcValue Args, result;
		Args = send_hex(send);

	try {
		xml_trace(2, "rig_cat_string send:", send.c_str());
		xml_trace(2, "                    ", ((std::string)(Args)).c_str());

		if (flrig_client->execute("rig.cat_string", Args, result)) {

			xml_trace(2, "rig.cat_string recv:", ((std::string)result).c_str());

			std::string res = recv_hex((std::string)result);

			return res;

		} else {
			return "?";
		}
	} catch (XmlRpcException *err) {
		xml_trace(2, "rig.cat_string: %s", err->getMessage().c_str());
		return "?";
	}
}

std::string client_get_xcvr()
{
	std::string _xcvr_name;
	XmlRpcValue result;
	try {
		if (flrig_client->execute("rig.get_xcvr", XmlRpcValue(), result) ) {
			_xcvr_name = std::string(result);
		}
	} catch (XmlRpcException *err) {
		xml_trace(2, "rig.get_xcvr: %s", err->getMessage().c_str());
		connected_to_client = false;
	}
	return _xcvr_name;
}

bool client_connection()
{
	XmlRpcValue noArgs, result;
	try {
		if (flrig_client->execute("system.listMethods", noArgs, result)) {
			int nargs = result.size();
			std::string method_str = "\nMethods:\n";
			for (int i = 0; i < nargs; i++)
				method_str.append("    ").append(result[i]).append("\n");

			connected_to_client = true;
			client_get_xcvr();
		} else {
			connected_to_client = false;
		}
	} catch (...) {
		return false;
	}
	return connected_to_client;
}

bool connect_to_client()
{
	XmlRpc::setVerbosity(xmlrpc_verbosity);
	if (flrig_client) {
		delete flrig_client;
		flrig_client = (XmlRpcClient *)0;
	}
	try {
		flrig_client = new XmlRpcClient(
				progStatus.xmlrig_addr.c_str(),
				atol(progStatus.xmlrig_port.c_str()));
#if !defined(__WIN32__) && !defined(__APPLE__)
		if (progStatus.use_UDS)
			flrig_client->useUDS();
#endif
		return client_connection();
	} catch (...) {
		xml_trace(3,"Cannot connect to %s : %s", progStatus.xmlrig_addr.c_str(), progStatus.xmlrig_port.c_str());
		return false;
	}
}
