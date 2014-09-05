#C++ Library of BTCChina Trade API
A C++ wrapper to trade bitcoin and litecoin via [BTCChina](https://www.btcchina.com) API.

##Installation
1.Download the library via:
```
git clone https://github.com/BTCChina/btcchina-api-cpp
```

2.Add the following files to your project:

 - base64.h base64.cpp
 - SHA1.h SHA1.cpp
 - HMAC_SHA1.h HMAC_SHA1.cpp
 - BTCChinaAPI.h BTCChinaAPI.cpp

3.Including "BTCChinaAPI.h"

4.Dependencies

 - [libcurl](http://curl.haxx.se/libcurl/)
 - C++ compiler supports C++11
   standard

##Usage
Create Trade API keys at https://vip.btcchina.com/account/apikeys, and set proper permissions as indicated.

Spawn CBTCChinaAPI instance with access key and secret key mentioned above. Notice that these keys cannot be modified later.

```C++
CBTCChinaAPI btcAPI(access_key, secret_key);
```

Call methods similiar to the format described in [API documentation](http://btcchina.org/api-trade-documentation-en). Results are string types passed to methods as the first parameter.

```C++
string result;
btcAPI.getAccountInfo(result);
```

##Returns
Every method returns the length of response on success, negative values for errors listed in btcERR enumeration below.

##Useful Enumerations
Enumerations can be iterated by _CBTCChinaAPI::enumeration_member_.
 
 - btcERR: curlERR | methodERR | contentERR | jsonRequestERR
 Negative values for returns.
 - MarketType: BTCCNY | LTCCNY | LTCBTC | ALL
 - CurrencyType: BTC | LTC
 - TransactionType: all | fundbtc | withdrawbtc | fundmoney | withdrawmoney | refundmoney | buybtc | sellbtc | buyltc | sellltc | tradefee | rebate

##Examples
###Get user information
```C++
btcAPI.getAccountInfo(result);
```

_Result_:
JSON Objects of [profile](http://btcchina.org/api-trade-documentation-en#profile), [balance](http://btcchina.org/api-trade-documentation-en#balance) and [frozen](http://btcchina.org/api-trade-documentation-en#frozen).

###Place order
```C++
btcAPI.placeOrder(string result, double price, double amount, MarketType market = BTCCNY);
```

Market type determines the precision of price and amount. See [FAQ](http://btcchina.org/api-trade-documentation-en#faq) No.6 for details.
_Parameters:_

- _price_: negative value to sell or buy at market price.
- _amount_: negative value to sell while positive value to buy.
- _market_: the market to place this order. Notice that ALL is not supported.

_Result:_
{"result":orderID} on success. [Invalid amount or invalid price](http://btcchina.org/api-trade-documentation-en#error_codes) error may occur.

###Cancel order
```C++
btcAPI.cancelOrder(string result, int orderID, MarketType market = BTCCNY);
```
_Parameters:_

- _orderID_: the ID returned by placeOrder method
- _market_: the market of the order placed previously. Notice that ALL is not supported.

_Result_:
{"result":true} if successful, otherwise {"result":false}

###Get Market Depth
```C++
btcAPI.getMarketDepth(string result, unsigned int limit = 10, MarketType market = BTCCNY);
```

Get the complete market depth.
_Parameters:_

- _limit_: number of orders returned per side.
- _market_: the market to get depth of. Notice that ALL is not supported.

_Result:_
[market_depth](http://btcchina.org/api-trade-documentation-en#market_depth) JSON object.

###Get Deposits
```C++
btcAPI.getDeposits(string result, CurrencyType currency, bool pendingonly = true);
```

Get all user deposits.

_Parameters:_

- _currency_: type of currency to get deposit records of.
- _pendingonly_: whether to get open deposits only.

_Result:_
Array of [deposit](http://btcchina.org/api-trade-documentation-en#deposit) JSON objects.

###Get Withdrawals
```C++
btcAPI.getWithdrawals(string result, CurrencyType currency, bool pendingonly = true);
```

Get all user withdrawals.

_Parameters:_

- _currency_: type of currency to get deposit records of.
- _pendingonly_: whether to get open withdrawals only.

_Result:_
[withdrawal](http://btcchina.org/api-trade-documentation-en#withdrawal) JSON object.

###Get single withdrawal status
```C++
btcAPI.getWithdrawal(string result, int withdrawalID, CurrencyType currency = BTC);
```

_Parameters:_

- _withdrawalID_: the withdrawal to get status of.
- _currency_: type of currency.

_Result:_
[withdrawal](http://btcchina.org/api-trade-documentation-en#withdrawal) JSON object.

###Request a withdrawal
```C++
btcAPI.requestWithdrawal(string result, CurrencyType currency, double amount);
```

Make a withdrawal request. BTC withdrawals will pick last used withdrawal address from user profile.

_Parameters:_

- _currency_: type of currency to withdraw.
- _amount_: amount of currency to withdraw

_Result:_
{"result":{"id":"withdrawalID"}}
Notice that the return format of withdrawalID is different from that of orderID.

###Get order status
```C++
btcAPI.getOrder(string result, unsigned int orderID, MarketType market = BTCCNY);
```

_Parameters:_

- _orderID_: the order to get status of.
- _market_: the market in which the order is placed. Notice that ALL is not supported.

_Result:_
[order](http://btcchina.org/api-trade-documentation-en#order) JSON object.

###Get all order status
```C++
btcAPI.getOrders(string result, bool openonly = true, MarketType market = BTCCNY, unsigned int limit = 1000, unsigned int offset = 0);
```

_Parameters:_

- _openonly_: whether to get open orders only.
- _market_: the market in which orders are placed.
- _limit_: the number of orders to show.
- _offset_: page index of orders.

_Result:_
Array of [order](http://btcchina.org/api-trade-documentation-en#order) JSON objects.

###Get transaction log
```C++
btcAPI.getTransactions(string result, TransactionType transaction = all, unsigned int limit = 10, unsigned int offset = 0);
```

Notice that prices returned by this method may differ from placeOrder as it is the price get procceeded.

_Parameters:_

- _transaction_: type of transaction to fetch.
- _limit_: the number ot transactions.
- _offset_: page index ot transactions.

_Result:_
Array of [transaction](http://btcchina.org/api-trade-documentation-en#transaction) JSON objects.




> Written with [StackEdit](https://stackedit.io/).