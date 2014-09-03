#include "socketio.hpp"

using namespace std;

int main(int argc, char* argv[])
{
	string uri = "websocket.btcchina.com/socket.io";
	if(argc>1)
	{
		uri = argv[1];
	}
	CSocketIOpp c(uri);
	c.start();
	//c.stop();
	return 0;
}