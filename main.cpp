#include <iostream>
#include<chrono>
#include<string>
#include<memory>
#include<thread>
#include "eventengine.h"
#include "gatewaymanager.h"
#include "algotrading.h"
#include "json11/json11.h"
#include "JSReply.h"
#include "spdlog/spdlog.h"
#include "vld.h"
class MainThread
{
public:
	MainThread()
	{
	}
	void run(EventEngine *eventengine)
	{
		std::shared_ptr<spdlog::logger> my_logger = spdlog::rotating_logger_mt("MainThread", "logs/server.txt", 1048576 * 10, 3);
		my_logger->flush_on(spdlog::level::info);
		JSSocket  socket(AF_SP, NN_REP);
		socket.bind("tcp://*:5555");

		Gatewaymanager gatewaymanager(eventengine);//接口管理器
		JSReply jsreply;
		AlgoTrading algotrading(&gatewaymanager, eventengine, &jsreply);
		eventengine->regEvent(EVENT_LOG, std::bind(&JSReply::showLog, &jsreply, std::placeholders::_1));
		eventengine->regEvent(EVENT_ERROR, std::bind(&JSReply::showError, &jsreply, std::placeholders::_1));
		eventengine->regEvent(EVENT_TICK, std::bind(&JSReply::onTick, &jsreply, std::placeholders::_1));
		eventengine->regEvent(EVENT_ACCOUNT, std::bind(&JSReply::onAccount, &jsreply, std::placeholders::_1));
		eventengine->regEvent(EVENT_POSITION, std::bind(&JSReply::onPosition, &jsreply, std::placeholders::_1));
		eventengine->regEvent(EVENT_ORDER, std::bind(&JSReply::onOrder, &jsreply, std::placeholders::_1));
		eventengine->regEvent(EVENT_TRADE, std::bind(&JSReply::onTrade, &jsreply, std::placeholders::_1));
		eventengine->regEvent(EVENT_LOADSTRATEGY, std::bind(&JSReply::onStrategyLoad, &jsreply, std::placeholders::_1));
		eventengine->regEvent(EVENT_UPDATESTRATEGY, std::bind(&JSReply::onUpdateStrategy, &jsreply, std::placeholders::_1));
		/*	eventengine.RegEvent(EVENT_UPDATEPORTFOLIO, std::bind(&JSTraderGUI::onPortfolioUpdate, this, std::placeholders::_1));
		*/
		eventengine->startEngine();
		while (true)
		{
			std::string msg = socket.recv();
			if (msg == "exit")
			{
				break;
			}

			std::cout << "receive msg:" << msg << std::endl;
			std::string err;
			const auto document = json11::Json::parse(msg, err);
			std::string msg_head = document["msg_head"].string_value();

			if (msg_head == "reqHistoryLog")
			{
				my_logger->info("reqHistoryLog");
				jsreply.publishHistoryLogs(&socket);
			}
			else if (msg_head == "reqHistoryChartData")
			{
				my_logger->info("reqHistoryChartData");
				jsreply.publishHistoryChartData(&socket);
			}
			else if (msg_head == "connect_ctp")
			{
				my_logger->info("connect_ctp");
				gatewaymanager.connect("ctp");
				socket.send("服务器收到命令:connect_ctp");
			}
			else if (msg_head == "subscribe_ctp")
			{
				my_logger->info("subscribe_ctp");
				jsstructs::SubscribeReq req;
				req.symbol = document["symbol"].string_value();
				gatewaymanager.subscribe(req, "ctp");
				socket.send("服务器收到指令:subscribe_ctp");
			}
			else if (msg_head == "sendorder_ctp")
			{
				my_logger->info("sendorder_ctp");
				jsstructs::OrderReq req = JSSocketConvert::getSendOrder(msg);
				req.exchange = gatewaymanager.getContract(req.symbol)->exchange;
				std::string orderID=gatewaymanager.sendOrder(req, "ctp");
				socket.send("服务器收到指令:sendorder_ctp orderID:"+orderID);
			}
			else if (msg_head == "cancelorder_ctp")
			{
				my_logger->info("cancelorder_ctp");
				jsstructs::CancelOrderReq req = JSSocketConvert::getCancelOrder(msg);
				req.exchange = gatewaymanager.getContract(req.symbol)->exchange;
				gatewaymanager.cancelOrder(req,"ctp");
				socket.send("服务器收到指令:cancelorder_ctp orderID:" + req.orderID);
			}
			else if (msg_head == "loadstrategy")
			{
				my_logger->info("loadstrategy");
				algotrading.loadStrategy();
				socket.send("服务器收到指令:loadstrategy");
			}
			else if (msg_head == "initstrategy")
			{
				my_logger->info("initstrategy");
				const std::string strategyname = JSSocketConvert::getStartStrategy(msg);
				algotrading.initStrategy(strategyname);
				socket.send("服务器收到指令:initstrategy" + strategyname);
			}
			else if (msg_head == "startstrategy")
			{
				my_logger->info("startstrategy");
				const std::string strategyname = JSSocketConvert::getStartStrategy(msg);
				algotrading.startStrategy(strategyname);
				socket.send("服务器收到指令:startstrategy" + strategyname);
			}
			else if (msg_head == "stopstrategy")
			{
				my_logger->info("stopstrategy");
				const std::string strategyname = JSSocketConvert::getStopStrategy(msg);
				algotrading.stopStrategy(strategyname);
				socket.send("服务器收到指令:stopstrategy" + strategyname);
			}
		}
	}
};


int main()
{
	if (!Utils::checkExist("./logs"))
	{
		if (Utils::createDirectory("./logs"))
		{
			std::cout << "create logs directory success" << std::endl;
		}
		else
		{
			std::cout << "create logs direcotry fail" << std::endl;
		}
	}
	EventEngine eventengine;//事件驱动引擎

	MainThread mainthread;
	std::thread t(std::mem_fn(&MainThread::run), mainthread, &eventengine);
	std::string line;
	while (getline(std::cin, line))
	{
		if (line == "exit")
		{
			eventengine.stopEngine();
			JSSocket s(AF_SP, NN_REQ);
			s.connect("tcp://127.0.0.1:5555");
			s.send("exit");
			t.join();
			break;
		}
	}

	return 0;
}