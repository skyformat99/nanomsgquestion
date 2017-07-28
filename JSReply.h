#ifndef JSREPLY_H
#define JSREPLY_H
#include "eventengine.h"
#include "jssocket.h"
#include "socketdatastruct.hpp"
#include "strategyTemplate.h"
#include "json11/json11.h"
#include <mutex>
class JSReply
{
public:
	JSReply();
	~JSReply();
	void showLog(std::shared_ptr<Event>e);
	void showError(std::shared_ptr<Event>e);
	void onTick(std::shared_ptr<Event>e);
	void onAccount(std::shared_ptr<Event>e);
	void onPosition(std::shared_ptr<Event>e);
	void onOrder(std::shared_ptr<Event>e);
	void onTrade(std::shared_ptr<Event>e);
	void onStrategyLoad(std::shared_ptr<Event>e);
	void onUpdateStrategy(std::shared_ptr<Event>e);
	void publishHistoryLogs(JSSocket *socket);
	void publishHistoryChartData(JSSocket *socket);

	void pushStrategyData(const jsstructs::BarData& bar, const jsstructs::BacktestGodData &data,bool inited);
private:
	std::mutex logmtx;

	JSSocket socket;

	JSSocket quotessocket;

	std::vector<std::string>logqueue;
	std::mutex ordermtx;
	std::vector < std::string>orderqueue;
	std::mutex dealmtx;
	std::vector<std::string>dealqueue;
	std::mutex strategymtx;
	std::vector<std::string>strategyqueue;
	std::vector <std::string>updateStrategyqueue;
	std::mutex chartmtx;
	std::map<std::string,std::vector<std::string>>strategyName_mapping_chartjsonqueue;
};
#endif