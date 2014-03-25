#include <iostream>
#include <math.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <sstream>
#include "HMAC_SHA1.h"
 
using namespace std;
 
// Reference:
// http://www.codeproject.com/Articles/22118/C-Class-Implementation-of-HMAC-SHA
string getHmacSha1(string key, string content) {
    unsigned char digest[20];
    CHMAC_SHA1 HMAC_SHA1;
    HMAC_SHA1.HMAC_SHA1((unsigned char *) content.c_str(), content.size(),
                        (unsigned char *) key.c_str(), key.size(), digest);
    stringstream output;
    char result[3];
    for (int i = 0; i < 20; i++) {
        sprintf(result, "%02x", digest[i]);
        output << result;
    }
    return output.str();
}
 
long long getMilliSeconds() {
    struct timeval start, end;
    gettimeofday( &start, NULL );
    return start.tv_sec * 1000000LL + start.tv_usec;
}
 
// Connects and gets the socket handle. Returns 0 if any errors.
int SocketConnect(string host_name, int port) {
    struct hostent* host = gethostbyname(host_name.c_str());
    int handle = socket(AF_INET, SOCK_STREAM, 0);
    if (handle == -1) {
        cout << "Error when creating socket to " << host_name;
        return 0;
    }
 
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr = *((in_addr *) host->h_addr);
    bzero(&(server.sin_zero), 8);
    int error = connect(handle, (sockaddr *) &server, sizeof(sockaddr));
    if (error == -1) {
        cout << "Error when connecting " << host_name;
        return 0;
    }
 
    return handle;
}
 
// Establishes a SSL connection. Return false if errors.
bool SslConnect(const string& server, int port,
                int& socket, SSL*& sslHandle, SSL_CTX*& sslContext) {
    socket = SocketConnect(server, port);
    if (socket == 0) {
        return false;
    }
 
    SSL_load_error_strings();
    SSL_library_init();
 
    // Use SSL 2 or 3, bind SSL to socket, and then do the SSL connection.
    if ((sslContext = SSL_CTX_new(SSLv23_client_method())) == NULL ||
        (sslHandle = SSL_new(sslContext)) == NULL ||
        SSL_set_fd(sslHandle, socket) != 1 ||
        SSL_connect(sslHandle) != 1) {
        ERR_print_errors_fp(stderr);
        return false;
    }
 
    return true;
}
 
string ReadAllFromSSL(SSL* ssl_handle) {
    const int BUCKET_READ_SIZE = 1024;
    char buffer[BUCKET_READ_SIZE];
    stringstream result;
    int received_bytes = std::numeric_limits<int>::max();
 
    while (received_bytes >= BUCKET_READ_SIZE) {
        received_bytes = SSL_read(ssl_handle, buffer, BUCKET_READ_SIZE);
        if (received_bytes > 0) {
            buffer[received_bytes] = '\0';
            result << buffer;
        } else {
            cout << "Error when read from SSL"; // To get the error reason, use SSL_get_error.
            return "";
        }
    }
    return result.str();
}
 
string Base64Encode(const string& message) {
    // Do encoding.
    BIO* bio = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    BIO_push(bio, bmem);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);  // // Ignore newlines
    BIO_write(bio, message.c_str(), message.size());
    BIO_flush(bio);
 
    // Get results.
    char* data;
    int length = BIO_get_mem_data(bmem, &data);
    string encoded = string(data, length);
    BIO_free_all(bio);
    return encoded;
}
 
string GetBtcPostContent(const string& accessKey, const string& secretKey,
                         const string& method) {
    // Get authorization token.
    long long tonce = getMilliSeconds();
    stringstream authInput;
    authInput << "tonce=" << tonce
              << "&accesskey=" << accessKey
              << "&requestmethod=post&id=1&method=" << method
              << "&params=";
    string paramsHash = getHmacSha1(secretKey, authInput.str());
    string authToken = Base64Encode(accessKey + ":" + paramsHash);
 
    // Get post content.
    string json_content = "{\"method\": \"getAccountInfo\", \"params\": [], \"id\": 1}";
    stringstream postStream;
    postStream << "POST /api_trade_v1.php HTTP/1.1\r\n"
               << "Content-Type: application/json-rpc\r\n"
               << "Authorization: Basic " << authToken << "\r\n"
               << "Json-Rpc-Tonce: " << tonce << "\r\n"
               << "Content-Length: " << json_content.size() << "\r\n"
               << "Host: api.btcchina.com\r\n\r\n"
               << json_content;
    string postContent = postStream.str();
    cout << "POST content: " << postStream.str() << endl;
    return postContent;
}
 
int main(int argc, char **argv) {
    string accessKey = "YOUR_ACCESS_KEY";
    string secretKey = "YOUR_SECRET_KEY";
    string method = "getAccountInfo";
    string postContent = GetBtcPostContent(accessKey, secretKey, method);
 
    // Start the real SSL request.
    int socket = 0;
    SSL* sslHandle = NULL;
    SSL_CTX* sslContext = NULL;
    if (SslConnect("api.btcchina.com", 443, socket, sslHandle, sslContext)) {
        SSL_write(sslHandle, postContent.c_str(), postContent.size());
        string response = ReadAllFromSSL(sslHandle);
        cout << "Get response: " << response << endl;
    }
 
    // Cleanups no matter connect succeed or not.
    if (socket != 0) {
        close(socket);
    }
    if (sslHandle) {
        SSL_shutdown(sslHandle);
        SSL_free(sslHandle);
    }
    if (sslContext) {
        SSL_CTX_free(sslContext);
    }
    return 0;
}
