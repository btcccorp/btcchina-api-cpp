#pragma once

#include <curl/curl.h>

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <random>

class CBTCChinaAPI
{
public:
	explicit CBTCChinaAPI(const std::string& access_key, const std::string& secret_key);
	~CBTCChinaAPI();
	//err list
	enum btcERR{ curlERR = -10, methodERR, contentERR, jsonRequestERR }; //every memeber < 0 for if clause
	enum marketList { BTCCNY = 0, LTCCNY, LTCBTC, ALL };
	enum currencyList { BTC = 0, LTC };
	enum transactionTypeList { all = 0, fundbtc, withdrawbtc, fundmoney, withdrawmoney, refundmoney, buybtc, sellbtc, buyltc, sellltc, tradefee, rebate };
	//interface
	int getAccountInfo(std::string& result);
	int PlaceOrder(std::string& result, double price, double amount, marketList market = BTCCNY);
	int cancelOrder(std::string& result, int orderID, marketList market = BTCCNY);
	int getOperations(std::string& result, bool operation, currencyList currency, bool pendingonly = true);
	int getMarketDepth(std::string& result, unsigned int limit = 10, marketList market = BTCCNY);
	int getWithdrawal(std::string& result, int withdrawalID, currencyList currency = BTC);
	int requestWithdrawal(std::string& result, currencyList currency, double amount);
	int getOrder(std::string& result, unsigned int orderID, marketList market = BTCCNY);
	int getOrders(std::string& result, bool openonly = true, marketList market = BTCCNY, unsigned int limit = 1000, unsigned int offset = 0);
	int getTransactions(std::string& result, transactionTypeList transaction = all, unsigned int limit = 10, unsigned int offset = 0);

private:
	//const
	static const std::string url;
	static const std::string postHeader;
	static const std::string markets[4];
	static const std::string currencies[2];
	static const std::string transactionTypes[12];
	//variables
	std::string accessKey, secretKey;
	CURL *curl;
	CURLcode res;
	//uncopyable
	CBTCChinaAPI(const CBTCChinaAPI&);
	CBTCChinaAPI &operator=(const CBTCChinaAPI&);
	//functions
	std::string getMillSec();
	std::string getHmacSha1(const std::string& key, const std::string& content);
	int DoMethod(const std::string& method, const std::string& mParams, std::string& result);
	//compensate for non-JSON signature hash format
	void replaceAll(std::string& str, const std::string& from, const std::string& to);
	std::mutex& sslMutex();
};

