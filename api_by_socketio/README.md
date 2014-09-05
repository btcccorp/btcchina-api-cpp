#btc-websocket-cpp
C++ client for socket.io-based [btcchina websocket api](http://btcchina.org/websocket-api-market-data-documentation-en).

This code snippet implemented few mechanisms of socket.io v1.0.x and it is not a fully functional client for socket.io v1.0.x.

###Dependencies
 - [boost](http://www.boost.org)
 - [websocketpp](https://github.com/zaphoyd/websocketpp)
 - [libcurl](http://curl.haxx.se/libcurl/)
 - [OpenSSL](https://www.openssl.org/)

###Build
cygwin:
```
g++ sample.cpp socketio.cpp -I../websocketpp -lcurl -lssl -lcrypto -lboost_system-mt -lboost_random-mt
```

###Special thanks
- https://github.com/betai/Xamarin.Socket.IO for the great work on handling socket.io handshake and messages in a minimalistic way.
- https://github.com/Wisembly/elephant.io/pull/53 for the inspiration on v1.0 changes.
 
> Written with [StackEdit](https://stackedit.io/).