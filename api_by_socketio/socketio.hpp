#include <string>
#include <sstream>
#include <iostream>
#include <curl/curl.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/unordered_map.hpp>
#include <boost/assign/list_of.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

class CSocketIOpp
{
public:
	CSocketIOpp(const std::string& uri);
	~CSocketIOpp();
	void start();
	void stop();
	void on_message(websocketpp::connection_hdl hdl, message_ptr msg);
	void on_open(websocketpp::connection_hdl hdl);
	void on_socket_init(websocketpp::connection_hdl hdl);
	context_ptr on_tls_init(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);
private:
	client m_socketio;
	websocketpp::connection_hdl m_hdl;
	bool getSocketioConfig();
	std::string m_sid;
	int m_pingInterval;
	int m_pingTimeout;
	std::string m_uri;
	const static std::string websocket_scheme, https_scheme, config_query, websocket_query;
	//v1.0 message types
	static boost::unordered_map<std::string, std::string> eioType;
	static boost::unordered_map<std::string, std::string> sioType;
};