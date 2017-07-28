#include "JSReply.h"
#include<thread>
JSReply::JSReply() :socket(AF_SP, NN_PUB), quotessocket(AF_SP, NN_PUB)
{
	this->socket.bind("tcp://*:6666");
	this->quotessocket.bind("tcp://*:7777");
}

JSReply::~JSReply()
{
}

void JSReply::showLog(std::shared_ptr<Event>e)
{
	std::unique_lock<std::mutex>lck(logmtx);
	std::shared_ptr<Event_Log> elog = std::static_pointer_cast<Event_Log>(e);
	std::string msg = "接口名:" + elog->gatewayname + "时间:" + elog->logTime + "信息:" + elog->msg;
	std::cout << Utils::UTF8_2_GBK(msg) << std::endl;
	std::string json = JSSocketConvert::callbackMSG(msg);
	if (this->logqueue.size() > 50)
	{
		this->logqueue.erase(this->logqueue.begin());
	}
	this->logqueue.push_back(json);
	this->socket.send(json);
}

void JSReply::showError(std::shared_ptr<Event>e)
{
	std::unique_lock<std::mutex>lck(logmtx);
	std::shared_ptr<Event_Error> eerror = std::static_pointer_cast<Event_Error>(e);
	std::string msg = Utils::UTF8_2_GBK("(Error)接口名:" + eerror->gatewayname + "时间:" + eerror->errorTime) + Utils::UTF8_2_GBK("编号") + eerror->errorID + Utils::UTF8_2_GBK("信息") + eerror->errorMsg;
	std::cout << msg << std::endl;
	std::string json = JSSocketConvert::callbackMSG("(Error)接口名:" + eerror->gatewayname + "时间:" + eerror->errorTime + "编号" + Utils::GBK_2_UTF8(eerror->errorID) + "信息" + Utils::GBK_2_UTF8(eerror->errorMsg));
	if (this->logqueue.size() > 50)
	{
		this->logqueue.erase(this->logqueue.begin());
	}
	this->logqueue.push_back(json);
	this->socket.send(json);
}

void JSReply::onTick(std::shared_ptr<Event>e)
{
	std::shared_ptr<Event_Tick> etick = std::static_pointer_cast<Event_Tick>(e);
	jsstructs::PriceTableData data;
	data.symbol = etick->symbol;
	data.lastPrice = std::to_string(etick->lastprice);
	data.bid = std::to_string(etick->bidprice1);
	data.ask = std::to_string(etick->askprice1);
	data.openInterest = std::to_string(etick->openInterest);
	data.upperLimit = std::to_string(etick->upperLimit);
	data.lowerLimit = std::to_string(etick->lowerLimit);
	data.datetime = etick->time;
	std::string json = JSSocketConvert::PriceTable(data);
	this->quotessocket.send(json);
}

void JSReply::onAccount(std::shared_ptr<Event>e)
{
	std::shared_ptr<Event_Account> eaccount = std::static_pointer_cast<Event_Account>(e);
	jsstructs::AccountData data;
	data.gatewayname = eaccount->gatewayname;
	data.accountid = eaccount->accountid;
	data.available = std::to_string(eaccount->available);
	data.balance = std::to_string(eaccount->balance);
	data.closeProfit = std::to_string(eaccount->closeProfit);
	data.commission = std::to_string(eaccount->commission);
	data.margin = std::to_string(eaccount->margin);
	data.positionProfit = std::to_string(eaccount->positionProfit);
	data.preBalance = std::to_string(eaccount->preBalance);
	std::string json = JSSocketConvert::AccountData(data, eaccount->gatewayname);
	this->socket.send(json);
}

void JSReply::onPosition(std::shared_ptr<Event>e)
{
	std::shared_ptr<Event_Position> eposition = std::static_pointer_cast<Event_Position>(e);
	jsstructs::PositionData data;
	data.gatewayname = eposition->gatewayname;
	data.symbol = eposition->symbol;
	data.direction = eposition->direction;
	data.frozen = std::to_string(eposition->frozen);
	data.position = std::to_string(eposition->position);
	data.price = std::to_string(eposition->price);
	data.todayPosition = std::to_string(eposition->todayPosition);
	data.todayPositionCost = std::to_string(eposition->todayPositionCost);
	data.ydPosition = std::to_string(eposition->ydPosition);
	data.ydPositionCost = std::to_string(eposition->ydPositionCost);
	std::string json = JSSocketConvert::PositionData(data, eposition->gatewayname);
	this->socket.send(json);
}

void JSReply::onOrder(std::shared_ptr<Event>e)
{
	std::unique_lock<std::mutex>lck(logmtx);
	std::shared_ptr<Event_Order> eorder = std::static_pointer_cast<Event_Order>(e);
	if (eorder->status == STATUS_CANCELLED)
	{
		std::string msg = "撤单消息发出！！"+eorder->orderID;
		std::cout << Utils::UTF8_2_GBK(msg) << std::endl;

		jsstructs::OrderCallback data;
		data.orderID = eorder->orderID;
		data.direction = eorder->direction;
		data.nodealvolume = std::to_string(eorder->totalVolume - eorder->tradedVolume);
		data.offset = eorder->offset;
		data.price = std::to_string(eorder->price);
		data.status = eorder->status;
		data.symbol = eorder->symbol;
		data.time = eorder->orderTime;
		data.volume = std::to_string(eorder->totalVolume);
		std::string json = JSSocketConvert::cancelordercallback(data, "ctp");
		this->socket.send(json);
		std::unique_lock<std::mutex>lck2(ordermtx);
		if (this->orderqueue.size() > 50)
		{
			this->orderqueue.erase(this->orderqueue.begin());
		}
		orderqueue.push_back(json);
	}
	else
	{
		jsstructs::OrderCallback data;
		data.orderID = eorder->orderID;
		data.direction = eorder->direction;
		data.nodealvolume = std::to_string(eorder->totalVolume - eorder->tradedVolume);
		data.offset = eorder->offset;
		data.price = std::to_string(eorder->price);
		data.status = eorder->status;
		data.symbol = eorder->symbol;
		data.time = eorder->orderTime;
		data.volume = std::to_string(eorder->totalVolume);
		std::string json = JSSocketConvert::ordercallback(data, "ctp");
		this->socket.send(json);
		std::unique_lock<std::mutex>lck2(ordermtx);
		if (this->orderqueue.size() > 50)
		{
			this->orderqueue.erase(this->orderqueue.begin());
		}
		orderqueue.push_back(json);
	}
}

void JSReply::onTrade(std::shared_ptr<Event>e)
{
	std::unique_lock<std::mutex>lck(dealmtx);
	std::shared_ptr<Event_Trade> etrade = std::static_pointer_cast<Event_Trade>(e);
	jsstructs::TradeCallback data;
	data.symbol = etrade->symbol;
	data.detailedstatus = "成交";
	data.direction = etrade->direction;
	data.offset = etrade->offset;
	data.price = std::to_string(etrade->price);
	data.time = etrade->tradeTime;
	data.volume = std::to_string(etrade->volume);
	std::string json = JSSocketConvert::tradecallback(data, "ctp");
	this->socket.send(json);
	if (this->dealqueue.size() > 50)
	{
		this->dealqueue.erase(this->dealqueue.begin());
	}
	this->dealqueue.push_back(json);
}
//strategies
void JSReply::onStrategyLoad(std::shared_ptr<Event>e)
{
	std::unique_lock<std::mutex>lck(strategymtx);
	std::shared_ptr<Event_LoadStrategy> eStrategy = std::static_pointer_cast<Event_LoadStrategy>(e);
	jsstructs::StrategyCallback data;
	data.strategyname = eStrategy->strategyname;
	data.parammap = eStrategy->parammap;
	data.varmap = eStrategy->varmap;
	std::string json= JSSocketConvert::strategycallback(data, "ctp");
	this->socket.send(json);
	this->strategyqueue.push_back(json);
}

void JSReply::onUpdateStrategy(std::shared_ptr<Event>e) 
{
	std::unique_lock<std::mutex>lck(strategymtx);
	std::shared_ptr<Event_UpdateStrategy> eStrategy = std::static_pointer_cast<Event_UpdateStrategy>(e);
	jsstructs::StrategyCallback data;
	data.strategyname = eStrategy->strategyname;
	data.parammap = eStrategy->parammap;
	data.varmap = eStrategy->varmap;
	std::string json = JSSocketConvert::updatestrategycallback(data, "ctp");
	this->socket.send(json);				//GUI
	if (this->updateStrategyqueue.size() >10)
	{
		this->updateStrategyqueue.erase(this->updateStrategyqueue.begin());
	}
	this->updateStrategyqueue.push_back(json);
}

void JSReply::publishHistoryLogs(JSSocket *socket)
{
	std::unique_lock<std::mutex>lck(logmtx);
	std::unique_lock<std::mutex>lck2(ordermtx);
	std::unique_lock<std::mutex>lck3(dealmtx);
	std::unique_lock<std::mutex>lck4(strategymtx);
	std::vector<std::string>msgqueue;
	for (std::vector<std::string>::const_iterator iter = this->logqueue.cbegin(); iter != this->logqueue.cend(); ++iter)
	{
		msgqueue.push_back(*iter);
	}
	for (std::vector<std::string>::const_iterator iter = this->orderqueue.cbegin(); iter != this->orderqueue.cend(); ++iter)
	{
		msgqueue.push_back(*iter);
	}
	for (std::vector<std::string>::const_iterator iter = this->dealqueue.cbegin(); iter != this->dealqueue.cend(); ++iter)
	{
		msgqueue.push_back(*iter);
	}

	for (std::vector<std::string>::const_iterator iter = this->strategyqueue.cbegin(); iter != this->strategyqueue.cend(); ++iter)
	{
		msgqueue.push_back(*iter);
	}

	for (std::vector<std::string>::const_iterator iter = this->updateStrategyqueue.cbegin(); iter != this->updateStrategyqueue.cend(); ++iter)
	{
		msgqueue.push_back(*iter);
	}

	socket->send(JSSocketConvert::MsgQueueToJson(msgqueue));
}

void JSReply::publishHistoryChartData(JSSocket *socket)
{
	std::unique_lock<std::mutex>lck(chartmtx);
//	std::this_thread::sleep_for(std::chrono::seconds(100));
	std::vector<std::string>candlestickqueue;
	for (std::map<std::string, std::vector<std::string>>::const_iterator iter = this->strategyName_mapping_chartjsonqueue.cbegin(); iter != this->strategyName_mapping_chartjsonqueue.cend(); ++iter)
	{
		for (std::vector<std::string> ::const_iterator it = iter->second.cbegin(); it != iter->second.cend(); ++it)
		{
			candlestickqueue.push_back(*it);
		}
	}
	socket->send(JSSocketConvert::CandlestickQueueToJson(candlestickqueue));
}

void JSReply::pushStrategyData(const jsstructs::BarData& bar, const jsstructs::BacktestGodData &data,bool inited)
{
	std::string json = JSSocketConvert::PlotData(data, bar, inited);
	this->quotessocket.send(json);
	std::unique_lock<std::mutex>lck(chartmtx);
	if (strategyName_mapping_chartjsonqueue.find(data.strategyname) == strategyName_mapping_chartjsonqueue.end())
	{
		std::vector<std::string>v;
		strategyName_mapping_chartjsonqueue[data.strategyname] = v;
	}

	if (strategyName_mapping_chartjsonqueue[data.strategyname].size() >= 1000)
	{
		strategyName_mapping_chartjsonqueue[data.strategyname].erase(strategyName_mapping_chartjsonqueue[data.strategyname].begin());
	}
	strategyName_mapping_chartjsonqueue[data.strategyname].push_back(json);
}