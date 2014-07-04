#include "BTCChinaAPI.h"
#include "base64.h"

#define SHA1_NO_UTILITY_FUNCTIONS //disable fopen in SHA1
#include "HMAC_SHA1.h"



using namespace std;

//init static members
const string CBTCChinaAPI::url = "https://api.btcchina.com/api_trade_v1.php";
const string CBTCChinaAPI::markets[4] = { "BTCCNY", "LTCCNY", "LTCBTC", "ALL" };
const string CBTCChinaAPI::currencies[2] = { "BTC", "LTC" };
const string CBTCChinaAPI::transactionTypes[12] = { "all", "fundbtc", "withdrawbtc", "fundmoney", "withdrawmoney", "refundmoney", "buybtc", "sellbtc", "buyltc", "sellltc", "tradefee", "rebate" };
//non-member helper functions
//1.random generator helper
int randGen()
{
	static random_device rd;
	static mt19937 mt(rd());
	static uniform_int_distribution<int> dist(1, 100000);
	return dist(mt);
}
//2.curl callback handler
static size_t postCallBack(void *content, size_t size, size_t nmemb, void *userp)
{
	(*(string *)userp) += (char*)content;
	return size*nmemb;
}

CBTCChinaAPI::CBTCChinaAPI(const string& access_key, const string& secret_key) :accessKey(access_key), secretKey(secret_key), curl(NULL)
{
	
	//setup curl library, check if curl=null every time.
	curl = curl_easy_init();
}


CBTCChinaAPI::~CBTCChinaAPI()
{
	curl_easy_cleanup(curl);
}

string CBTCChinaAPI::getHmacSha1(const string& key, const string& content)
{
	unsigned char digest[20];
	CHMAC_SHA1 HMAC_SHA1;
	HMAC_SHA1.HMAC_SHA1((unsigned char *)content.c_str(), content.length(),
		(unsigned char *)key.c_str(), key.size(), digest);
	stringstream hashBuilder;
	for (int i = 0; i < 20; i++)
		hashBuilder << hex << setfill('0') << setw(2) << (unsigned int)digest[i];
	return hashBuilder.str();
}
//get million seconds in windows, thanks to http://www.openasthra.com/wp-content/uploads/gettimeofday.c, which I can't open at the moment.
//notice that the original unix epoch value is not correct: https://code.google.com/p/tesseract-ocr/issues/detail?id=631#c16
//unix users have their handy gettimeofday().
string CBTCChinaAPI::getMillSec()
{
	const unsigned long long unixEpoch = 116444736000000000ULL;
	FILETIME ft;
	unsigned long long tmpT = 0;
	GetSystemTimeAsFileTime(&ft);
	//get accurate time in high/low bits
	tmpT |= ft.dwHighDateTime;
	tmpT = tmpT << 32;
	tmpT |= ft.dwLowDateTime;
	//convert to unix million seconds time
	tmpT -= unixEpoch;
	stringstream timeFormat;
	timeFormat << tmpT / 10;
	return timeFormat.str();
}

//core function
//try to make local every connection-related variables.
int CBTCChinaAPI::DoMethod(const string& method, const string& mParams, string& result)
{
	lock_guard<mutex> _(sslMutex());
	if (curl)
	{
		//get random number
		string methodID = to_string(randGen());
		//compensate for non-JSON format signature
		string modified_mParams(mParams);
		replaceAll(modified_mParams, "true", "1");
		replaceAll(modified_mParams, "false", "");
		replaceAll(modified_mParams, "\"", "");
		// Get authorization token.
		string tonce = getMillSec();
		stringstream authInput;
		authInput << "tonce=" << tonce
			<< "&accesskey=" << accessKey
			<< "&requestmethod=post"
			<< "&id=" << methodID
			<< "&method=" << method
			<< "&params=" << modified_mParams;
		string paramsHash = getHmacSha1(secretKey, authInput.str());
		
		//thanks to ReneNyffenegger at https://github.com/ReneNyffenegger/development_misc/tree/master/base64 for base64encoding
		string authString = accessKey + ":" + paramsHash;
		string authToken = base64_encode((unsigned char*)authString.c_str(), authString.length());

		// Get json string for post content.
		stringstream jsonBuilder;
		jsonBuilder << "{\"method\": \"" << method
			<< "\", \"params\": [" << mParams
			<< "], \"id\": " << methodID
			<< "}";
		string json_content = jsonBuilder.str();
		//Build headers
		struct curl_slist *httpHeaders = NULL;
		httpHeaders = curl_slist_append(httpHeaders, "Content-Type: application/json-rpc");
		httpHeaders = curl_slist_append(httpHeaders, ("Authorization: Basic " + authToken).c_str());
		httpHeaders = curl_slist_append(httpHeaders, ("Json-Rpc-Tonce: " + tonce).c_str());
		//httpHeaders = curl_slist_append(httpHeaders, "Content-Length: " + json_content.length()); -->curl strlen(postfields) automatically
		httpHeaders = curl_slist_append(httpHeaders, "Host: api.btcchina.com");

		//set curl_option
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpHeaders);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_content.c_str());
		//setup callback options to read response
		string response = "";
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, postCallBack);
		//for http error
		char curlErrBuffer[CURL_ERROR_SIZE];
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlErrBuffer);
		//execute curl operation
		res = curl_easy_perform(curl);
		curl_slist_free_all(httpHeaders);
		if (res != CURLE_OK)
		{
			result = curlErrBuffer;
			result += method;
			return curlERR;
		}

		//remove http response header
		result = response;
		int p = 0;
		p = result.find_first_of('{');
		if (p < 0)
		{
			result = "Trade API error:\n" + response;
			return jsonRequestERR;
		}
		result = result.substr(p);
		//compare returned json-request-id
		p = result.find_last_of(':');
		if (result.substr(p + 2, result.length() - p - 4) != methodID)//format: ,"id":"xxx"}
		{
			result = "JSON-request ID doesn't match between server and client.\n" + method;
			return jsonRequestERR;
		}
		p = result.find_last_of(',');
		result = result.substr(0, p) + "}";
		return result.length();
	}
	else
	{
		curl = curl_easy_init();
		result = "error invoke curl, please try again.\n" + method;
		return curlERR;
	}
}

void CBTCChinaAPI::replaceAll(string& str, const string& from, const string& to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

mutex& CBTCChinaAPI::sslMutex()
{
	static mutex m;
	return m;
}

//public interfaces
int CBTCChinaAPI::getAccountInfo(string& result)
{
	string method = "getAccountInfo";
	string mParams = "";
	return DoMethod(method, mParams, result);
}
//negative price is considered as market price, negative amount as sell, and positive amount as buy. default market is BTCCNY
int CBTCChinaAPI::PlaceOrder(string& result, double price, double amount, marketList market)
{
	stringstream orderBuilder;
	orderBuilder << setiosflags(ios::fixed);
	string method = "", mParams = "";
	switch (market)
	{
	case BTCCNY:
		orderBuilder << setprecision(2) << price << "," << setprecision(4) << amount;
		break;
	case LTCCNY:
		orderBuilder << setprecision(2) << price << "," << setprecision(3) << amount;
		break;
	case LTCBTC:
		orderBuilder << setprecision(4) << price << "," << setprecision(3) << amount;
		break;
	default://"ALL" is not supported
		result = "Market not supported.\n";
		return contentERR;
	}
	mParams = orderBuilder.str();
	if (price < 0)
		mParams.replace(0, mParams.find_first_of(','), "null");
	if (amount < 0)
	{
		mParams = mParams.erase(mParams.find_first_of('-'), 1);
		method = "sellOrder2";
	}
	else
	{
		method = "buyOrder2";
	}

	//not default market
	if (market != BTCCNY)
		mParams += ",\"" + markets[market] + "\"";

	return DoMethod(method, mParams, result);
}
int CBTCChinaAPI::cancelOrder(string& result, int orderID, marketList market)
{
	string method = "cancelOrder";
	string mParams = to_string(orderID);
	//all is not supported
	if (market == ALL)
	{
		result = "Market:ALL is not supported.";
		return contentERR;
	}
	//not default market
	if (market != BTCCNY)
		mParams += ",\"" + markets[market] + "\"";

	return DoMethod(method, mParams, result);
}
//operation=true for getDeposits, false for getWithdrawals
int CBTCChinaAPI::getOperations(string& result, bool operation, currencyList currency, bool pendingonly)
{
	string method = "";
	if (operation)
		method = "getDeposits";
	else
		method = "getWithdrawals";
	string mParams = "\"" + currencies[currency] + "\"";
	if (!pendingonly)
		mParams += ",false";
	return DoMethod(method, mParams, result);
}
int CBTCChinaAPI::getMarketDepth(string& result, unsigned int limit, marketList market)
{
	string method = "getMarketDepth2";
	string mParams = "";
	if (limit != 10) mParams = to_string(limit);
	if (market != BTCCNY)
		mParams += ",\"" + markets[market] + "\"";
	return DoMethod(method, mParams, result);
}
int CBTCChinaAPI::getWithdrawal(string& result, int withdrawalID, currencyList currency)
{
	string method = "getWithdrawal";
	string mParams = to_string(withdrawalID);
	if (currency != BTC)
		mParams += ",\"" + currencies[currency] + "\"";//should be "LTC" but for further implmentations
	return DoMethod(method, mParams, result);
}
int CBTCChinaAPI::requestWithdrawal(string& result, currencyList currency, double amount)
{
	if (amount <= 0)
	{
		result = "withdrawal amount cannot be negative nor zero";
		return contentERR;
	}
	string method = "requestWithdrawal";
	stringstream paraBuilder;
	paraBuilder << setiosflags(ios::fixed);
	paraBuilder << "\"" << currencies[currency] << "\"," << setprecision(3) << amount;
	return DoMethod(method, paraBuilder.str(), result);
}
int CBTCChinaAPI::getOrder(string& result, unsigned int orderID, marketList market)
{
	if (market == ALL)
	{
		result = "Market: ALL is not supported.";
		return contentERR;
	}
	else
	{
		string method = "getOrder";
		string mParams = to_string(orderID);
		if (market != BTCCNY)
			mParams += ",\"" + markets[market] + "\"";
		return DoMethod(method, mParams, result);
	}
}
int CBTCChinaAPI::getOrders(string& result, bool openonly, marketList market, unsigned int limit, unsigned int offset)
{
	//due to the complexity of parameters, all default values are explicitly set.
	string method = "getOrders";
	stringstream paraBuilder;
	paraBuilder << setiosflags(ios::boolalpha) << openonly << ",\"" << markets[market] << "\"," << limit << "," << offset;
	return DoMethod(method, paraBuilder.str(), result);
}
int CBTCChinaAPI::getTransactions(string& result, transactionTypeList transaction, unsigned int limit, unsigned int offset)
{
	//likewise, set all parameters
	string method = "getTransactions";
	stringstream paraBuilder;
	paraBuilder << "\"" << transactionTypes[transaction] << "\"," << limit << "," << offset;
	return DoMethod(method, paraBuilder.str(), result);
}