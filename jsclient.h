#ifndef JSCLIENT_H
#define JSCLIENT_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QTextBrowser>
#include <QGridLayout>
#include <QCheckBox>
#include <QTableView>
#include <QGroupBox>
#include <QHeaderView>
#include <QScrollArea>
#include <QMenu>
#include <QMenuBar>
#include <QStandardItemModel>
#include <QCloseEvent>
#include <QMessageBox>
#include <QtWebEngineWidgets/qwebengineview.h>
#include <QtWebChannel/qwebchannel.h>
#include <QThread>

#include<string>
#include<iostream>
#include "socketdatastruct.hpp"
#include "Orderwidget.h"
#include "jssocket.h"
#include "callback.h"
#include "Quotesthread.h"
#include "transferobject.h"
extern const std::string g_ServerAddress;
class ChartWindow :public QMainWindow
{
	Q_OBJECT
public:
	ChartWindow()
	{
		this->m_transferobject = new TransferObject;
		QWebEngineView *view = new QWebEngineView;
		view->resize(1920, 1080);
		view->showMaximized();
		view->setUrl(QUrl("qrc:///var.html"));
		QWebChannel*channel = new QWebChannel(this);
		channel->registerObject(QStringLiteral("transferobject"), this->m_transferobject);
		view->page()->setWebChannel(channel);
		this->setCentralWidget(view);
		this->showMaximized();
	}
	~ChartWindow()
	{
		delete this->m_transferobject;
	}
	inline  TransferObject * transferobject() const
	{
		return this->m_transferobject;
	}
private:
	TransferObject *m_transferobject;
};


class JSClient : public QMainWindow
{
    Q_OBJECT
signals:
	void writeLog(QString msg);
	void startListenMsg();
	void startListenQuotes();
	void sendVarData(const jsstructs::PlotDataStruct &plotdata);
	void updateAccount(const jsstructs::AccountData& data);
	void updatePosition(const jsstructs::PositionData&data);
	void updateOrders(const jsstructs::OrderCallback &data);
	void updateTrades(const jsstructs::TradeCallback &data);
	void updateLoadedStrategy(const jsstructs::StrategyCallback &data);
	void updateStrategyParamVar(const jsstructs::StrategyCallback &data);
public:
    explicit JSClient(QWidget *parent = 0);
    ~JSClient();
private:
	JSSocket socket;

	QWidget *portfolioManageWidget;
	QWidget *orderWidget;


	JSCallback *jscallback;
	QThread *callbackThread;

	JSQuotesthread *jsquotesthread;
	QThread *quotesthread;

	void loadUI();
	QGroupBox *createLogGroup();
	QGroupBox *createAccountGroup();
	QGroupBox *createPositionGroup();
	QGroupBox *createControlGroup();
	QScrollArea *createStrategyGroup();
	//QGroupBox *createPortfolio();

	void closeEvent(QCloseEvent   *  event);
	QStandardItemModel accountModel;
	QStandardItemModel positionModel;
 
	QGridLayout *strategyLayout;

	std::map<QCheckBox*, std::string>checkbox_mapping_strategyname;
	std::map<std::string, QStandardItemModel*>strategyname_mapping_varmodel;
	std::map<std::string, QStandardItemModel*>strategyname_mapping_parammodel;

	std::map<std::string, ChartWindow*>strategyname_mapping_chartwindow;

	bool is_loadchartdata=false;

	private slots:

	void onPressConnectCTP();
	void onPressLoadStrategy();
	void onPressInitStrategy();
	void onPressStartStrategy();
	void onPressStopStrategy();
	void onPressChartShow();
	void onPressPortfolioShow();
	void onPressOrderwidgetShow();
	void onPressReqHistoryLog();
	void onPressReqHistoryChart();
	void updateAccountBox(const jsstructs::AccountData &data);
	void updatePositionBox(const jsstructs::PositionData &data);
	void createStrategyBox(const jsstructs::StrategyCallback &data);
	void updateStrategyBox(const jsstructs::StrategyCallback &data);
	void updateVarPlot(const  jsstructs::PlotDataStruct &data);


	//void UpdatePortfolioBox(PortfolioData data);

};

#endif // MAINWINDOW_H
