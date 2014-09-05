#include "socketio.hpp"

using namespace std;


const string CSocketIOpp::websocket_scheme = "wss://";
const string CSocketIOpp::https_scheme = "https://";
const string CSocketIOpp::config_query = "/?transport=polling";
const string CSocketIOpp::websocket_query = "/?transport=websocket";

boost::unordered_map<string, string> CSocketIOpp::eioType = boost::assign::map_list_of
	(string("OPEN"), string("0"))
	(string("CLOSE"), string("1"))
	(string("PING"), string("2"))
	(string("PONG"), string("3"))
	(string("MESSAGE"), string("4"))
	(string("UPGRADE"), string("5"))
	(string("NOOP"), string("6"));

boost::unordered_map<string, string> CSocketIOpp::sioType = boost::assign::map_list_of
	(string("CONNECT"), string("0"))
	(string("DISCONNECT"), string("1"))
	(string("EVENT"), string("2"))
	(string("ACK"), string("3"))
	(string("ERROR"), string("4"))
	(string("BINARY_EVENT"), string("5"))
	(string("BINARY_ACK"), string("6"));


static size_t writeCallBack(void *content, size_t size, size_t nmemb, void *userp)
{
	string temp((char*)content, size*nmemb);
	(*(string *)userp) += temp;
	return size*nmemb;
}

bool CSocketIOpp::getSocketioConfig()
{
	CURL* curl = curl_easy_init();
	bool result = false;
	if(curl)
	{
		char curlErrBuffer[CURL_ERROR_SIZE];
		string response = "";
		string url = https_scheme + m_uri + config_query;

		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallBack);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlErrBuffer);
		CURLcode res = curl_easy_perform(curl);
		if(res != CURLE_OK)
		{
			cerr << "error read:" << url << endl << "message:" << curlErrBuffer;
		}
		else
		{
			boost::property_tree::ptree wsConfigJSON;
			stringstream reader(response.substr(response.find_first_of('{'), response.find_last_of('}') - response.find_first_of('{') + 1));
			boost::property_tree::read_json(reader, wsConfigJSON);
			result = true;
			//save variables, ignore upgrades array for now.
			m_sid = wsConfigJSON.get<string>("sid"); 
			m_pingTimeout = wsConfigJSON.get<int>("pingTimeout");
			m_pingInterval = wsConfigJSON.get<int>("pingInterval");
		}
	}
	curl_easy_cleanup(curl);
	curl = NULL;
	return result;
}

CSocketIOpp::CSocketIOpp(const string& uri):m_uri(uri)
{
	m_socketio.clear_access_channels(websocketpp::log::alevel::all);
    m_socketio.set_access_channels(websocketpp::log::alevel::connect);
    m_socketio.set_access_channels(websocketpp::log::alevel::disconnect);
    m_socketio.set_access_channels(websocketpp::log::alevel::app);

	m_socketio.init_asio();

	m_socketio.set_open_handler(bind(&CSocketIOpp::on_open,this,::_1));
	m_socketio.set_close_handler(bind(&CSocketIOpp::on_close,this,::_1));
	m_socketio.set_message_handler(bind(&CSocketIOpp::on_message,this,::_1,::_2));
	m_socketio.set_socket_init_handler(bind(&CSocketIOpp::on_socket_init,this,::_1));
	m_socketio.set_tls_init_handler(bind(&CSocketIOpp::on_tls_init,this,::_1));
}

CSocketIOpp::~CSocketIOpp()
{
	m_socketio.close(m_hdl, websocketpp::close::status::going_away, "");
}

void CSocketIOpp::on_open(websocketpp::connection_hdl hdl)
{
	cout << "opened." << endl;
	m_hdl = hdl;
	m_socketio.send(hdl, eioType["UPGRADE"] + sioType["EVENT"], websocketpp::frame::opcode::text);
}

void CSocketIOpp::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
	string payload = msg->get_payload();
	string eio = payload.substr(0, 1);
	//todo: add PING/PONG accordingly. A timer is necessary.
	if(eio == eioType["MESSAGE"])
	{
		string sio = payload.substr(1, 1);
		if(sio == sioType["CONNECT"])
			m_socketio.send(hdl, eioType["MESSAGE"] + 
								sioType["EVENT"] + 
								"[\"subscribe\",[\"marketdata_cnybtc\",\"marketdata_cnyltc\",\"marketdata_btcltc\"]]",
								websocketpp::frame::opcode::text);
		else if (sio == sioType["EVENT"])
		{
			if(payload.substr(4, 5) == "trade")//listen on "trade"
				cout << payload.substr(payload.find_first_of('{'), payload.find_last_of('}') - payload.find_first_of('{') + 1) << endl;
		}
	}
	else if(eio == eioType["PING"])
	{
		m_socketio.send(hdl, eioType["PONG"] + payload.substr(1, payload.length() - 1), websocketpp::frame::opcode::text);
	}
}

void CSocketIOpp::on_socket_init(websocketpp::connection_hdl hdl)
{
	cout << "socket inited." << endl;
}

context_ptr CSocketIOpp::on_tls_init(websocketpp::connection_hdl hdl)
{
	cout << "tls inited." << endl;
	context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));
	try {
		ctx->set_options(boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::single_dh_use);
	} catch (exception& e) {
		cerr << e.what() << endl;
	}
	return ctx;
}

void CSocketIOpp::on_close(websocketpp::connection_hdl hdl)
{
	cout << "websocket closed." << endl;
}

void CSocketIOpp::start()
{
	if(!getSocketioConfig())
	{
		cerr << "can't get socketio config at:" << https_scheme + m_uri + config_query << endl;
		return;
	}
	websocketpp::lib::error_code ec;
	client::connection_ptr con = m_socketio.get_connection(websocket_scheme + m_uri + websocket_query + "&sid=" + m_sid, ec);
	if(ec)
	{
		cerr << "Connection error:" << ec.message() << endl;
		return;
	}
	m_socketio.connect(con);
	m_socketio.run();
}

void CSocketIOpp::stop()
{
	m_socketio.send(m_hdl, eioType["MESSAGE"] + sioType["DISCONNECT"], websocketpp::frame::opcode::text);
}