#include"jsclient.h"
#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif
extern const std::string g_ServerAddress;
JSClient::JSClient(QWidget *parent) :QMainWindow(parent), socket(AF_SP, NN_REQ)
{
	this->setWindowIcon(QIcon(":/client.ico"));
	this->setWindowTitle("jsclient");
	this->resize(1024, 800);
	socket.connect(g_ServerAddress + ":5555");//5555port is reqrep  6666 is pubsub
	callbackThread = new QThread;
	quotesthread = new QThread;
	jscallback = new JSCallback();
	jsquotesthread = new JSQuotesthread();
	loadUI();

	connect(this, SIGNAL(startListenMsg()), jscallback, SLOT(listen_msg()), Qt::QueuedConnection);
	connect(this, SIGNAL(startListenQuotes()), jsquotesthread, SLOT(listen_quotes()), Qt::QueuedConnection);
	qRegisterMetaType<jsstructs::PriceTableData>("jsstructs::PriceTableData");//注册到元系统中
	qRegisterMetaType<jsstructs::AccountData>("jsstructs::AccountData");//注册到元系统中
	qRegisterMetaType<jsstructs::PositionData>("jsstructs::PositionData");//注册到元系统中
	qRegisterMetaType<jsstructs::OrderCallback>("jsstructs::OrderCallback");//注册到元系统中
	qRegisterMetaType<jsstructs::TradeCallback>("jsstructs::TradeCallback");//注册到元系统中
	qRegisterMetaType<jsstructs::StrategyCallback>("jsstructs::StrategyCallback");//注册到元系统中
	qRegisterMetaType<jsstructs::PlotDataStruct>("jsstructs::PlotDataStruct");//注册到元系统中

	connect(jscallback, SIGNAL(updateOrders(const  jsstructs::OrderCallback &)), this->orderWidget, SLOT(onOrder(const jsstructs::OrderCallback &)));
	connect(jscallback, SIGNAL(updateTrades(const  jsstructs::TradeCallback &)), this->orderWidget, SLOT(onTrade(const jsstructs::TradeCallback &)), Qt::QueuedConnection);
	connect(jscallback, SIGNAL(updateLoadedStrategy(const jsstructs::StrategyCallback &)), this, SLOT(createStrategyBox(const jsstructs::StrategyCallback &)), Qt::QueuedConnection);
	connect(jscallback, SIGNAL(updateStrategyParamVar(const jsstructs::StrategyCallback &)), this, SLOT(updateStrategyBox(const jsstructs::StrategyCallback &)), Qt::QueuedConnection);

	connect(jsquotesthread, SIGNAL(sendLastPrice(const  jsstructs::PriceTableData &)), this->orderWidget, SLOT(updatePrice(const  jsstructs::PriceTableData &)), Qt::QueuedConnection);
	connect(jsquotesthread, SIGNAL(sendVarData(const jsstructs::PlotDataStruct &)), this, SLOT(updateVarPlot(const  jsstructs::PlotDataStruct &)), Qt::QueuedConnection);
	connect(jscallback, SIGNAL(updateAccount(const  jsstructs::AccountData &)), this, SLOT(updateAccountBox(const  jsstructs::AccountData &)), Qt::QueuedConnection);
	connect(jscallback, SIGNAL(updatePosition(const  jsstructs::PositionData &)), this, SLOT(updatePositionBox(const  jsstructs::PositionData &)), Qt::QueuedConnection);
	connect(this, SIGNAL(sendVarData(const jsstructs::PlotDataStruct &)), this, SLOT(updateVarPlot(const  jsstructs::PlotDataStruct &)), Qt::QueuedConnection);
	connect(this, SIGNAL(updateOrders(const  jsstructs::OrderCallback &)), this->orderWidget, SLOT(onOrder(const jsstructs::OrderCallback &)));
	connect(this, SIGNAL(updateTrades(const  jsstructs::TradeCallback &)), this->orderWidget, SLOT(onTrade(const jsstructs::TradeCallback &)), Qt::QueuedConnection);
	connect(this, SIGNAL(updateLoadedStrategy(const jsstructs::StrategyCallback &)), this, SLOT(createStrategyBox(const jsstructs::StrategyCallback &)), Qt::QueuedConnection);
	connect(this, SIGNAL(updateStrategyParamVar(const jsstructs::StrategyCallback &)), this, SLOT(updateStrategyBox(const jsstructs::StrategyCallback &)), Qt::QueuedConnection);


	jscallback->moveToThread(callbackThread);
	callbackThread->start();

	jsquotesthread->moveToThread(quotesthread);
	quotesthread->start();

	emit startListenMsg();
	emit startListenQuotes();
}

JSClient::~JSClient()
{
	for (std::map<std::string, ChartWindow*>::iterator iter = strategyname_mapping_chartwindow.begin(); iter != strategyname_mapping_chartwindow.end(); ++iter)
	{
		delete iter->second;
		iter->second = nullptr;
	}

	delete jscallback;
	delete jsquotesthread;

	callbackThread->quit();
	callbackThread->wait();

	quotesthread->quit();
	quotesthread->wait();

	delete callbackThread;
	delete quotesthread;

	delete this->portfolioManageWidget;
	delete this->orderWidget;

	for (std::map<std::string, QStandardItemModel*>::iterator it = this->strategyname_mapping_varmodel.begin(); it != this->strategyname_mapping_varmodel.end(); ++it)
	{
		delete it->second;
		it->second = nullptr;
	}

	for (std::map<std::string, QStandardItemModel*>::iterator it = this->strategyname_mapping_parammodel.begin(); it != this->strategyname_mapping_parammodel.end(); ++it)
	{
		delete it->second;
		it->second = nullptr;
	}

}

void JSClient::closeEvent(QCloseEvent   *  event)
{
	QMessageBox::StandardButton button;
	button = QMessageBox::question(this, ("退出程序"),
		QString(("警告：程序有一个任务正在运行中，是否结束操作退出?")),
		QMessageBox::Yes | QMessageBox::No);

	if (button == QMessageBox::No) {
		event->ignore();  //忽略退出信号，程序继续运行
	}
	else if (button == QMessageBox::Yes) {
		event->accept();  //接受退出信号，程序退出
	}
}

void JSClient::loadUI()
{
	//tab控件
	QTabWidget *maintabwidget = new QTabWidget();
	QWidget *tab1widget = new QWidget();//账户信息页
	QWidget *tab2widget = new QWidget();//策略信息页
	maintabwidget->addTab(tab1widget, "账户信息");
	maintabwidget->addTab(tab2widget, "策略信息");

	//第一页
	QGridLayout *grid = new QGridLayout;
	grid->addWidget(createAccountGroup(), 0, 0);
	grid->addWidget(createPositionGroup(), 1, 0);
	grid->addWidget(createLogGroup(), 2, 0);
	tab1widget->setLayout(grid);


	//第二页
	QGridLayout *grid2 = new QGridLayout;
	QWidget* control = createControlGroup();
	control->setFixedHeight(80);
	grid2->addWidget(control, 0, 0);
	grid2->addWidget(createStrategyGroup(), 1, 0);
	grid2->setRowStretch(0, 1);
	grid2->setRowStretch(1, 5);
	tab2widget->setLayout(grid2);

	//菜单
	QMenuBar *menubar = this->menuBar();
	QMenu *connectmenu = menubar->addMenu("连接接口");
	connectmenu->addAction("连接CTP", this, SLOT(onPressConnectCTP()));
	menubar->addAction("组合管理", this, SLOT(onPressPortfolioShow()));
	menubar->addAction("手工下单", this, SLOT(onPressOrderwidgetShow()));
	menubar->addAction("读取历史服务器消息", this, SLOT(onPressReqHistoryLog()));
	menubar->addAction("读取历史K线图", this, SLOT(onPressReqHistoryChart()));

	this->setCentralWidget(maintabwidget);


	this->portfolioManageWidget = new QWidget;
	//QGridLayout *grid3 = new QGridLayout;
	//grid3->addWidget(this->createPortfolio());
	//this->portfolioManageWidget->setLayout(grid3);
	this->portfolioManageWidget->resize(800, 600);

	this->orderWidget = new Orderwidget(&this->socket);
}

QGroupBox *JSClient::createLogGroup()
{
	QGroupBox *groupBox = new QGroupBox("日志");
	QTextBrowser *Logbrowser = new QTextBrowser;
	QGridLayout *grid = new QGridLayout;
	grid->addWidget(Logbrowser);
	groupBox->setLayout(grid);
	connect(jscallback, SIGNAL(writeLog(QString)), Logbrowser, SLOT(append(QString)));
	connect(this, SIGNAL(writeLog(QString)), Logbrowser, SLOT(append(QString)));
	return groupBox;
}
//账户
QGroupBox *JSClient::createAccountGroup()
{
	QGroupBox *groupBox = new QGroupBox("账户");
	QStringList accountheader;
	accountheader << ("接口名") << ("账户ID") << ("昨结") << ("净值") << ("可用") << ("手续费") << ("保证金") << ("平仓盈亏") << ("持仓盈亏");
	this->accountModel.setHorizontalHeaderLabels(accountheader);
	QTableView *tableView = new QTableView;
	tableView->setModel(&this->accountModel);
	tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tableView->setSelectionBehavior(QAbstractItemView::SelectRows);  //单击选择一行  
	tableView->setSelectionMode(QAbstractItemView::SingleSelection); //设置只能选择一行，不能多行选中  
	tableView->setAlternatingRowColors(true);
	QGridLayout *grid = new QGridLayout;
	grid->addWidget(tableView);
	groupBox->setLayout(grid);
	return groupBox;
}

QGroupBox *JSClient::createPositionGroup()
{
	QGroupBox *groupBox = new QGroupBox("持仓");
	QStringList positionheader;
	positionheader << ("接口名") << ("合约") << ("方向") << ("仓位") << ("昨仓") << ("冻结资金") << ("持仓价");
	this->positionModel.setHorizontalHeaderLabels(positionheader);
	QTableView *PositionView = new QTableView;
	PositionView->setModel(&this->positionModel);
	PositionView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	PositionView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	PositionView->setSelectionBehavior(QAbstractItemView::SelectRows);  //单击选择一行  
	PositionView->setSelectionMode(QAbstractItemView::SingleSelection); //设置只能选择一行，不能多行选中  
	PositionView->setAlternatingRowColors(true);
	QGridLayout *grid = new QGridLayout;
	grid->addWidget(PositionView);
	groupBox->setLayout(grid);
	return groupBox;
}

QGroupBox *JSClient::createControlGroup()
{
	QGroupBox *groupBox = new QGroupBox("控制");
	QPushButton *load = new QPushButton(("加载策略"));
	QPushButton *init = new QPushButton(("初始化"));
	QPushButton *start = new QPushButton(("启动策略"));
	QPushButton *stop = new QPushButton(("停止策略"));
	QPushButton *chart = new QPushButton("显示图像");

	connect(load, SIGNAL(clicked()), this, SLOT(onPressLoadStrategy()));
	connect(init, SIGNAL(clicked()), this, SLOT(onPressInitStrategy()));
	connect(start, SIGNAL(clicked()), this, SLOT(onPressStartStrategy()));
	connect(stop, SIGNAL(clicked()), this, SLOT(onPressStopStrategy()));
	connect(chart, SIGNAL(clicked()), this, SLOT(onPressChartShow()));

	QHBoxLayout *Hbox = new QHBoxLayout;
	Hbox->addWidget(load);
	Hbox->addWidget(init);
	Hbox->addWidget(start);
	Hbox->addWidget(stop);
	Hbox->addWidget(chart);
	Hbox->addStretch();
	groupBox->setLayout(Hbox);
	return groupBox;
}

QScrollArea *JSClient::createStrategyGroup()//创建策略栏
{

	QScrollArea *scrollarea = new QScrollArea();
	QWidget *w = new QWidget;
	strategyLayout = new QGridLayout;
	w->setLayout(strategyLayout);
	scrollarea->setWidget(w);
	scrollarea->setWidgetResizable(true);
	return scrollarea;
}

//QGroupBox *JSClient::createPortfolio()
//{
//QGroupBox *groupBox = new QGroupBox("策略");
//QGridLayout *layout = new QGridLayout;
//QTableView *tableview = new QTableView;
//tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
//tableview->setEditTriggers(QAbstractItemView::NoEditTriggers);
//tableview->setSelectionBehavior(QAbstractItemView::SelectRows);  //单击选择一行  
//tableview->setSelectionMode(QAbstractItemView::SingleSelection); //设置只能选择一行，不能多行选中  
//tableview->setAlternatingRowColors(true);
//QStringList portfolioheader;
//portfolioheader << ("Dll名字") << ("策略") << ("合约") << ("盈利") << ("亏损") << ("净盈利") << ("持仓盈亏") << ("总盈亏") << ("持仓") << ("均价") << ("回撤") << ("今日盈亏");
//this->.setHorizontalHeaderLabels(portfolioheader);
//tableview->setModel(&this->positionModel);
//layout->addWidget(tableview);
//groupBox->setLayout(layout);
//return groupBox;
//}

void JSClient::onPressConnectCTP()
{
	std::string json = JSSocketConvert::connect("ctp");
	this->socket.send(json);
	std::string result = this->socket.recv();
	emit writeLog(QString::fromStdString(result));
}

void JSClient::onPressLoadStrategy()
{
	std::string json = JSSocketConvert::loadStrategy();
	this->socket.send(json);
	std::string result = this->socket.recv();
	emit writeLog(QString::fromStdString(result));
}

void JSClient::onPressInitStrategy()
{
	is_loadchartdata = true;
	for (std::map<QCheckBox*, std::string>::const_iterator it = this->checkbox_mapping_strategyname.cbegin(); it != this->checkbox_mapping_strategyname.cend(); ++it)
	{
		//遍历一次整个框框 
		if (it->first->isChecked() == true)
		{
			std::string json = JSSocketConvert::initStrategy(it->second);
			this->socket.send(json);
			std::string result = this->socket.recv();
			emit writeLog(QString::fromStdString(result));
		}
	}
}

void JSClient::onPressStartStrategy()
{
	for (std::map<QCheckBox*, std::string>::const_iterator it = this->checkbox_mapping_strategyname.cbegin(); it != this->checkbox_mapping_strategyname.cend(); ++it)
	{
		//遍历一次整个框框 
		if (it->first->isChecked() == true)
		{
			std::string json = JSSocketConvert::startStrategy(it->second);
			this->socket.send(json);
			std::string result = this->socket.recv();
			emit writeLog(QString::fromStdString(result));
		}
	}
}

void JSClient::onPressStopStrategy()
{
	for (std::map<QCheckBox*, std::string>::const_iterator it = this->checkbox_mapping_strategyname.cbegin(); it != this->checkbox_mapping_strategyname.cend(); ++it)
	{
		//遍历一次整个框框 
		if (it->first->isChecked() == true)
		{
			std::string json = JSSocketConvert::stopStrategy(it->second);
			this->socket.send(json);
			std::string result = this->socket.recv();
			emit writeLog(QString::fromStdString(result));
		}
	}
}

void JSClient::onPressChartShow()
{
	for (std::map<QCheckBox*, std::string>::const_iterator it = this->checkbox_mapping_strategyname.cbegin(); it != this->checkbox_mapping_strategyname.cend(); ++it)
	{
		//遍历一次整个框框 
		if (it->first->isChecked() == true)
		{
			strategyname_mapping_chartwindow.at(checkbox_mapping_strategyname.at(it->first))->show();
		}
	}
}

void JSClient::onPressPortfolioShow()
{
	this->portfolioManageWidget->show();
}

void JSClient::onPressOrderwidgetShow()
{
	this->orderWidget->show();
}

void JSClient::onPressReqHistoryLog()
{
	std::string json = JSSocketConvert::reqHistoryLog();
	this->socket.send(json);
	json = this->socket.recv();

	std::vector<std::string>msgqueue = JSSocketConvert::JsonToMsgQueue(json);
	for (std::vector<std::string>::const_iterator iter = msgqueue.cbegin(); iter != msgqueue.cend(); ++iter)
	{
		std::string msg = *iter;
		if (msg.find("msg_head") != std::string::npos)
		{
			//解包
			std::string err;
			const auto document = json11::Json::parse(msg, err);
			std::string msg_head = document["msg_head"].string_value();

			if (msg_head == "reply_log")
			{
				emit writeLog(QString::fromStdString(document["msg_data"].string_value()));
			}
			else if (msg_head == "ordercallback_ctp")
			{
				emit updateOrders(JSSocketConvert::getOrdercallback(msg));
			}
			else if (msg_head == "cancelordercallback_ctp")
			{
				emit updateOrders(JSSocketConvert::getCancelordercallback(msg));
			}
			else if (msg_head == "tradecallback_ctp")
			{
				emit updateTrades(JSSocketConvert::getTradecallback(msg));
			}
			else if (msg_head == "strategycallback_ctp")
			{
				emit updateLoadedStrategy(JSSocketConvert::getStrategy(msg));
			}
			else if (msg_head == "updatestrategy_ctp")
			{
				emit updateStrategyParamVar(JSSocketConvert::getStrategy(msg));
			}
		}
	}
	emit writeLog("接收历史消息完成");
}

void JSClient::onPressReqHistoryChart()
{
	is_loadchartdata = true;
	std::string json = JSSocketConvert::reqHistoryChart();
	qDebug() << "395:";
	this->socket.send(json);
	qDebug() << "397:";
	json = this->socket.recv();
	qDebug() << "399:" + QString::fromStdString(json);
	std::vector<std::string>msgqueue = JSSocketConvert::JsonToCandlestickQueue(json);
	for (std::vector<std::string>::const_iterator iter = msgqueue.cbegin(); iter != msgqueue.cend(); ++iter)
	{

		std::string msg = *iter;
		if (msg.find("msg_head") != std::string::npos)
		{
			//解包
			std::string err;
			const auto document = json11::Json::parse(msg, err);
			std::string msg_head = document["msg_head"].string_value();
			if (msg_head == "plotdata")
			{
				emit writeLog(QString::fromStdString(msg));
				emit  sendVarData(JSSocketConvert::getPlotData(msg));
			}
		}
	}
	emit writeLog("接收历史K线完成");
}

void JSClient::updateAccountBox(const jsstructs::AccountData &data)
{
	const int rowCount = this->accountModel.rowCount();
	if (rowCount == 0)
	{
		this->accountModel.setItem(0, 0, new QStandardItem(QString::fromStdString(data.gatewayname)));
		this->accountModel.setItem(0, 1, new QStandardItem(QString::fromStdString(data.accountid)));
		this->accountModel.setItem(0, 2, new QStandardItem(QString::fromStdString(data.preBalance)));
		this->accountModel.setItem(0, 3, new QStandardItem(QString::fromStdString(data.balance)));
		this->accountModel.setItem(0, 4, new QStandardItem(QString::fromStdString(data.available)));
		this->accountModel.setItem(0, 5, new QStandardItem(QString::fromStdString(data.commission)));
		this->accountModel.setItem(0, 6, new QStandardItem(QString::fromStdString(data.margin)));
		this->accountModel.setItem(0, 7, new QStandardItem(QString::fromStdString(data.closeProfit)));
		this->accountModel.setItem(0, 8, new QStandardItem(QString::fromStdString(data.positionProfit)));
	}
	for (int i = 0; i < rowCount; ++i)
	{
		if ((this->accountModel.item(i, 0)->text() == QString::fromStdString(data.gatewayname)) && (this->accountModel.item(i, 1)->text() == QString::fromStdString(data.accountid)))
		{
			this->accountModel.setItem(i, 0, new QStandardItem(QString::fromStdString(data.gatewayname)));
			this->accountModel.setItem(i, 1, new QStandardItem(QString::fromStdString(data.accountid)));
			this->accountModel.setItem(i, 2, new QStandardItem(QString::fromStdString(data.preBalance)));
			this->accountModel.setItem(i, 3, new QStandardItem(QString::fromStdString(data.balance)));
			this->accountModel.setItem(i, 4, new QStandardItem(QString::fromStdString(data.available)));
			this->accountModel.setItem(i, 5, new QStandardItem(QString::fromStdString(data.commission)));
			this->accountModel.setItem(i, 6, new QStandardItem(QString::fromStdString(data.margin)));
			this->accountModel.setItem(i, 7, new QStandardItem(QString::fromStdString(data.closeProfit)));
			this->accountModel.setItem(i, 8, new QStandardItem(QString::fromStdString(data.positionProfit)));
			break;
		}
		else if (i == rowCount - 1)
		{
			this->accountModel.setItem(rowCount, 0, new QStandardItem(QString::fromStdString(data.gatewayname)));
			this->accountModel.setItem(rowCount, 1, new QStandardItem(QString::fromStdString(data.accountid)));
			this->accountModel.setItem(rowCount, 2, new QStandardItem(QString::fromStdString(data.preBalance)));
			this->accountModel.setItem(rowCount, 3, new QStandardItem(QString::fromStdString(data.balance)));
			this->accountModel.setItem(rowCount, 4, new QStandardItem(QString::fromStdString(data.available)));
			this->accountModel.setItem(rowCount, 5, new QStandardItem(QString::fromStdString(data.commission)));
			this->accountModel.setItem(rowCount, 6, new QStandardItem(QString::fromStdString(data.margin)));
			this->accountModel.setItem(rowCount, 7, new QStandardItem(QString::fromStdString(data.closeProfit)));
			this->accountModel.setItem(rowCount, 8, new QStandardItem(QString::fromStdString(data.positionProfit)));
		}
	}
}

void JSClient::updatePositionBox(const jsstructs::PositionData &data)
{
	const int rowCount = this->positionModel.rowCount();
	QString direction;
	if (data.direction == DIRECTION_SHORT)
	{
		direction = "空";
	}
	else if (data.direction == DIRECTION_LONG)
	{
		direction = "多";
	}

	if (rowCount == 0)
	{
		this->positionModel.setItem(0, 0, new QStandardItem(QString::fromStdString(data.gatewayname)));
		this->positionModel.setItem(0, 1, new QStandardItem(QString::fromStdString(data.symbol)));
		if (data.direction == DIRECTION_SHORT)
		{
			this->positionModel.setItem(0, 2, new QStandardItem("空"));
			this->positionModel.item(0, 2)->setForeground(QBrush(Qt::green));
		}
		else if (data.direction == DIRECTION_LONG)
		{
			this->positionModel.setItem(0, 2, new QStandardItem("多"));
			this->positionModel.item(0, 2)->setForeground(QBrush(Qt::red));
		}
		this->positionModel.setItem(0, 3, new QStandardItem(QString::fromStdString(data.position)));
		this->positionModel.setItem(0, 4, new QStandardItem(QString::fromStdString(data.ydPosition)));
		this->positionModel.setItem(0, 5, new QStandardItem(QString::fromStdString(data.frozen)));
		this->positionModel.setItem(0, 6, new QStandardItem(QString::fromStdString(data.price)));

	}
	for (int i = 0; i < rowCount; ++i)
	{
		if ((this->positionModel.item(i, 1)->text() == QString::fromStdString(data.symbol)) && (this->positionModel.item(i, 2)->text() == direction))
		{
			this->positionModel.setItem(i, 0, new QStandardItem(QString::fromStdString(data.gatewayname)));
			this->positionModel.setItem(i, 1, new QStandardItem(QString::fromStdString(data.symbol)));
			this->positionModel.setItem(i, 2, new QStandardItem(direction));
			if (direction == "多")
			{
				this->positionModel.item(i, 2)->setForeground(QBrush(Qt::red));
			}
			else
			{
				this->positionModel.item(i, 2)->setForeground(QBrush(Qt::green));
			}
			this->positionModel.setItem(i, 3, new QStandardItem(QString::fromStdString(data.position)));
			this->positionModel.setItem(i, 4, new QStandardItem(QString::fromStdString(data.ydPosition)));
			this->positionModel.setItem(i, 5, new QStandardItem(QString::fromStdString(data.frozen)));
			this->positionModel.setItem(i, 6, new QStandardItem(QString::fromStdString(data.price)));
			break;
		}
		else if (i == rowCount - 1)
		{
			this->positionModel.setItem(rowCount, 0, new QStandardItem(QString::fromStdString(data.gatewayname)));
			this->positionModel.setItem(rowCount, 1, new QStandardItem(QString::fromStdString(data.symbol)));
			if (data.direction == DIRECTION_SHORT)
			{
				this->positionModel.setItem(rowCount, 2, new QStandardItem("空"));
				this->positionModel.item(rowCount, 2)->setForeground(QBrush(Qt::green));
			}
			else if (data.direction == DIRECTION_LONG)
			{
				this->positionModel.setItem(rowCount, 2, new QStandardItem("多"));
				this->positionModel.item(rowCount, 2)->setForeground(QBrush(Qt::red));
			}
			this->positionModel.setItem(rowCount, 3, new QStandardItem(QString::fromStdString(data.position)));
			this->positionModel.setItem(rowCount, 4, new QStandardItem(QString::fromStdString(data.ydPosition)));
			this->positionModel.setItem(rowCount, 5, new QStandardItem(QString::fromStdString(data.frozen)));
			this->positionModel.setItem(rowCount, 6, new QStandardItem(QString::fromStdString(data.price)));
		}
	}
}

void JSClient::createStrategyBox(const jsstructs::StrategyCallback &data)
{
	//接收CTAMANAGER处理的策略putgui的事件
	QGroupBox *strategybox = new QGroupBox(QString::fromStdString(data.strategyname));
	strategyLayout->addWidget(strategybox);
	//创建绘图对象
	ChartWindow *chartwindow = new ChartWindow;
	chartwindow->setWindowTitle(QString::fromStdString(data.strategyname));
	strategyname_mapping_chartwindow[data.strategyname] = chartwindow;

	QStandardItemModel *param_model = new QStandardItemModel;
	QStandardItemModel *var_model = new QStandardItemModel;

	QCheckBox *checkbox = new QCheckBox("选择此策略");

	QTableView *param_tableview = new QTableView;
	param_tableview->setModel(param_model);
	param_tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	param_tableview->setEditTriggers(QAbstractItemView::NoEditTriggers);
	param_tableview->verticalHeader()->hide();

	QTableView *var_tableview = new QTableView;
	var_tableview->setModel(var_model);
	var_tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	var_tableview->setEditTriggers(QAbstractItemView::NoEditTriggers);
	var_tableview->verticalHeader()->hide();

	this->strategyname_mapping_parammodel.insert(std::pair<std::string, QStandardItemModel*>(data.strategyname, param_model));
	this->strategyname_mapping_varmodel.insert(std::pair<std::string, QStandardItemModel*>(data.strategyname, var_model));
	this->checkbox_mapping_strategyname.insert(std::pair<QCheckBox*, std::string>(checkbox, data.strategyname));

	//参数设置
	QStringList paramhead;
	for (std::map<std::string, std::string>::const_iterator it = data.parammap.cbegin(); it != data.parammap.cend(); ++it)
	{
		paramhead << QString::fromStdString(it->first);
		param_model->setItem(0, param_model->columnCount(), new QStandardItem(QString::fromStdString(it->second)));
	}
	param_model->setHorizontalHeaderLabels(paramhead);
	param_tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	//变量设置
	QStringList varhead;
	for (std::map<std::string, std::string>::const_iterator it = data.varmap.cbegin(); it != data.varmap.cend(); ++it)
	{
		varhead << QString::fromStdString(it->first);
		var_model->setItem(0, var_model->columnCount(), new QStandardItem(QString::fromStdString(it->second)));
	}

	var_model->setHorizontalHeaderLabels(varhead);
	var_tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	QVBoxLayout *vboxlayout = new QVBoxLayout;
	vboxlayout->addWidget(checkbox);
	vboxlayout->addWidget(param_tableview);
	vboxlayout->addWidget(var_tableview);
	strategybox->setLayout(vboxlayout);
}

void JSClient::updateStrategyBox(const jsstructs::StrategyCallback &data)
{
	if (this->strategyname_mapping_varmodel.find(data.strategyname) != this->strategyname_mapping_varmodel.end())
	{
		QStandardItemModel *var_model = this->strategyname_mapping_varmodel[data.strategyname];
		int i = 0;
		for (std::map<std::string, std::string>::const_iterator it = data.varmap.cbegin(); it != data.varmap.cend(); ++it)
		{
			var_model->setItem(0, i, new QStandardItem(QString::fromStdString(it->second)));
			++i;
		}
	}
}

void JSClient::updateVarPlot(const  jsstructs::PlotDataStruct &data)
{
	if (is_loadchartdata == true)
	{
		json11::Json::array bar{ data.bar.open, data.bar.close, data.bar.low, data.bar.high };
		json11::Json::object  mainchart;
		json11::Json::object indicator;
		for (std::map<std::string, double>::const_iterator it = data.data.mainchartMap.cbegin(); it != data.data.mainchartMap.cend(); ++it)
		{
			mainchart[it->first] = it->second;
		}

		for (std::map<std::string, double>::const_iterator it = data.data.indicatorMap.cbegin(); it != data.data.indicatorMap.cend(); ++it)
		{
			indicator[it->first] = it->second;
		}

		json11::Json jsonobj = json11::Json::object{
			{ "datetime", data.bar.date + data.bar.time },
			{ "bar", bar },
			{ "mainchart", mainchart },
			{ "indicator", indicator }
		};

		std::string json_str = jsonobj.dump();

		if (strategyname_mapping_chartwindow.find(data.data.strategyname) != strategyname_mapping_chartwindow.end())
		{
			if (data.inited == false)
			{
				//not inited load data not plot
				strategyname_mapping_chartwindow[data.data.strategyname]->transferobject()->sendInited(QString::fromStdString(json_str));
			}
			else
			{
				//inited load data and plot
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				strategyname_mapping_chartwindow[data.data.strategyname]->transferobject()->setSendVar(QString::fromStdString(json_str));
			}
		}
	}
}
