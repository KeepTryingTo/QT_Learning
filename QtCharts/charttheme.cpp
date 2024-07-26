#include "charttheme.h"
#include "ui_charttheme.h"

#include <QRandomGenerator>

ChartTheme::ChartTheme(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ChartTheme)
{
    ui->setupUi(this);

    this -> chartQLineSeries();
    this -> chartQSPlineSeries();
    this -> chartQBarSeries();
    this -> chartQScatterSeries();

    this -> chartQPieSeries();
    this -> chartQBarMaxMinSeries();
    this -> chartQCandleSeries();
    this -> chartQAreaSeries();


    this -> populateAnimationBox();
    this -> populateThemeBox();
    this -> populateLegendBox();
    this -> populateChartBox();

    //选择不同的主题颜色
    connect(ui -> themeComboBox,&QComboBox::currentIndexChanged,this,[=](int index){
        this -> themeIndex = index;
        this -> updateUI();
    });
    //选择不同的动画
    connect(ui -> animatedComboBox,&QComboBox::currentIndexChanged,this,[=](int index){
        this -> AnimationIndex = index;
        this -> updateUI();
    });
    //选择不同的图例
    connect(ui -> legendComboBox,&QComboBox::currentIndexChanged,this,[=](int index){
        this -> legendIndex = index;
        this -> updateUI();
    });
    //选择不同的图表显示在当前画布中
    connect(ui -> chartComboBox,&QComboBox::currentIndexChanged,this,&ChartTheme::selectCharts);
}

ChartTheme::~ChartTheme()
{
    delete ui;
}

void ChartTheme::populateThemeBox()
{
    // add items to theme combobox
    ui->themeComboBox->addItem("Light", QChart::ChartThemeLight);
    ui->themeComboBox->addItem("Blue Cerulean", QChart::ChartThemeBlueCerulean);
    ui->themeComboBox->addItem("Dark", QChart::ChartThemeDark);
    ui->themeComboBox->addItem("Brown Sand", QChart::ChartThemeBrownSand);
    ui->themeComboBox->addItem("Blue NCS", QChart::ChartThemeBlueNcs);
    ui->themeComboBox->addItem("High Contrast", QChart::ChartThemeHighContrast);
    ui->themeComboBox->addItem("Blue Icy", QChart::ChartThemeBlueIcy);
    ui->themeComboBox->addItem("Qt", QChart::ChartThemeQt);
}

void ChartTheme::populateAnimationBox()
{
    // add items to animation combobox
    ui->animatedComboBox->addItem("No Animations", QChart::NoAnimation);
    ui->animatedComboBox->addItem("GridAxis Animations", QChart::GridAxisAnimations);
    ui->animatedComboBox->addItem("Series Animations", QChart::SeriesAnimations);
    ui->animatedComboBox->addItem("All Animations", QChart::AllAnimations);
}

void ChartTheme::populateLegendBox()
{
    // add items to legend combobox
    ui->legendComboBox->addItem("No Legend ", 0);
    ui->legendComboBox->addItem("Legend Top", Qt::AlignTop);
    ui->legendComboBox->addItem("Legend Bottom", Qt::AlignBottom);
    ui->legendComboBox->addItem("Legend Left", Qt::AlignLeft);
    ui->legendComboBox->addItem("Legend Right", Qt::AlignRight);


}

void ChartTheme::populateChartBox()
{
    // add items to charts combobox
    ui->chartComboBox->addItem("All Default ", 0);
    ui->chartComboBox->addItem("QLineSeries", 1);
    ui->chartComboBox->addItem("QSplineSeries", 2);
    ui->chartComboBox->addItem("QBarSeries", 3);
    ui->chartComboBox->addItem("QPieSeries", 4);
    ui->chartComboBox->addItem("QCandleSeries", 5);
    ui->chartComboBox->addItem("QBarMinMaxSeries", 6);
    ui->chartComboBox->addItem("QScatterSeries", 7);
    ui->chartComboBox->addItem("QAreaSeries", 8);
}

QChartView *  ChartTheme::chartQLineSeries()
{
    QLineSeries * series = new QLineSeries();
    for(int i = 0 ; i < 10; i++){//添加坐标点
        series -> append(i,1.1 * i);
    }

    QChart * chart = new QChart();
    // chart -> legend() -> hide();
    chart -> setTitle("QLineSeries");
    chart -> addSeries(series);
    chart -> createDefaultAxes();//创建默认的坐标轴，也可以自定义坐标轴
    chart -> axes(Qt::Vertical).first() -> setRange(0,30);
    chart -> axes(Qt::Horizontal).first() -> setRange(0,12);
    chart -> setVisible(true);

    QChartView * chartView = new QChartView(chart);
    chartView -> setRenderHint(QPainter::Antialiasing); //启用抗锯齿效果
    chartView -> setVisible(true);

    if(this -> chartIndex == 0) {
        ui -> gridLayout -> addWidget(chartView,0,0);
    }
    this -> m_charts . append(chartView);
    return chartView;
}

QChartView *  ChartTheme::chartQSPlineSeries()
{
    QSplineSeries * splines = new QSplineSeries();
    for(int i = 0 ; i < 10; i++){
        int y = QRandomGenerator::global() -> bounded(30);
        splines -> append(i,y);
    }


    QChart * chart_splines = new QChart();
    // chart_splines -> legend() -> hide();
    chart_splines -> setTitle("QSPlineSeries");
    chart_splines -> addSeries(splines);
    chart_splines -> createDefaultAxes();//创建默认的坐标轴，也可以自定义坐标轴
    chart_splines -> axes(Qt::Vertical).first() -> setRange(0,30);
    chart_splines -> axes(Qt::Horizontal).first() -> setRange(0,12);
    chart_splines -> setVisible(true);

    QChartView * chartView_splines = new QChartView(chart_splines);
    chartView_splines -> setRenderHint(QPainter::Antialiasing); //启用抗锯齿效果
    chartView_splines -> setVisible(true);


    if(this -> chartIndex == 0) {
        ui -> gridLayout -> addWidget(chartView_splines,0,1);
    }

    this -> m_charts . append(chartView_splines);
    return chartView_splines;
}

QChartView *  ChartTheme::chartQBarSeries()
{
    QBarSeries * barseries = new QBarSeries();

    QBarSet * set_0 = new QBarSet("Qt with number");
    for(int i = 0 ; i < 5; i++){
        int random_number = QRandomGenerator::global() -> bounded(100);
        set_0 -> append(random_number);
    }
    set_0 -> setColor(Qt::red);

    barseries -> append(set_0);

    QBarSet * set_1 = new QBarSet("Qt with rate");
    for(int i = 0 ; i < 5; i++){
        int random_number = QRandomGenerator::global() -> bounded(100);
        set_1 -> append(random_number);
    }
    set_1 -> setColor(Qt::cyan);

    barseries -> append(set_1);

    QBarSet * set_2 = new QBarSet("Qt with grade");
    for(int i = 0 ; i < 5; i++){
        int random_number = QRandomGenerator::global() -> bounded(100);
        set_2 -> append(random_number);
    }
    set_2 -> setColor(Qt::blue);

    barseries -> append(set_2);

    QChart * chart = new QChart();
    chart -> addSeries(barseries);
    chart -> setTitle("Grade of Student");
    chart -> setAnimationOptions(QChart::SeriesAnimations);

    QStringList SubjectName;
    SubjectName.append("English");
    SubjectName.append("Math");
    SubjectName.append("Physics");
    SubjectName.append("Chemistry");
    SubjectName.append("Language");

    QBarCategoryAxis * axisX = new QBarCategoryAxis();
    axisX -> append(SubjectName);
    chart -> addAxis(axisX,Qt::AlignBottom);
    barseries -> attachAxis(axisX);

    QValueAxis * axisY = new QValueAxis();
    axisY -> setRange(0,100);
    chart -> addAxis(axisY,Qt::AlignLeft);
    barseries -> attachAxis(axisY);

    chart -> legend() -> setVisible(true);
    chart -> legend() -> setAlignment(Qt::AlignBottom);

    QChartView * chartView = new QChartView(chart);
    chartView -> setRenderHint(QPainter::Antialiasing);


    if(this -> chartIndex == 0) {
        ui -> gridLayout -> addWidget(chartView,0,2);
    }

    this -> m_charts . append(chartView);
    return chartView;
}

QChartView *ChartTheme::chartQScatterSeries()
{
    QScatterSeries * series = new QScatterSeries();
    for(int i = 0 ; i < 10; i++){
        int rand = QRandomGenerator::global() -> bounded(10);
        series -> append(i,rand);
    }

    // 设置散点标签（可选）
    series->setPointLabelsVisible(true);
    series->setPointLabelsFormat("(@xPoint, @yPoint)");

    // 设置散点形状和大小（可选）
    series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    series->setMarkerSize(10.0);

    QChart * chart = new QChart();
    chart -> addSeries(series);
    chart -> setTitle("Scatter Example");
    chart -> setAnimationOptions(QChart::AllAnimations); //设置动画效果
    chart -> legend() -> setVisible(true);

    QValueAxis * axisX = new QValueAxis();
    axisX -> setTitleText("random number");
    axisX -> setRange(0,12);
    chart -> addAxis(axisX,Qt::AlignBottom);
    series -> attachAxis(axisX);

    QValueAxis * axisY = new QValueAxis();
    axisY -> setTitleText("Value");
    axisY -> setRange(0,12);
    chart -> addAxis(axisY,Qt::AlignLeft);
    series -> attachAxis(axisY);

    QChartView * chartView = new QChartView(chart);
    chartView -> setRenderHint(QPainter::Antialiasing);
    chartView -> setVisible(true);


    if(this -> chartIndex == 0) {
        ui -> gridLayout -> addWidget(chartView,0,3);
    }

    return chartView;
}

QChartView *  ChartTheme::chartQPieSeries()
{
    QPieSeries * pieSeries = new QPieSeries();
    pieSeries -> append("English",10);
    pieSeries -> append("Math",20);
    pieSeries -> append("Pysics",20);
    pieSeries -> append("Language",30);
    pieSeries -> append("Chemistry",20);

    // pieSeries -> setHoleSize(0.25);//设置饼图中间洞的大小

    QPieSlice * slice0 = pieSeries -> slices().at(0);
    slice0 -> setExploded(true);
    slice0 -> setLabelVisible(true);
    slice0 -> setPen(QPen(Qt::red,1));
    slice0 -> setBrush(Qt::red);

    QPieSlice * slice1 = pieSeries -> slices().at(1);
    slice1 -> setExploded(true);
    slice1 -> setLabelVisible(true);
    slice1 -> setPen(QPen(Qt::blue,1));
    slice1 -> setBrush(Qt::blue);

    QPieSlice * slice3 = pieSeries -> slices().at(3);
    slice3 -> setExploded(true);
    slice3 -> setLabelVisible(true);
    slice3 -> setPen(QPen(Qt::cyan,1));
    slice3 -> setBrush(Qt::cyan);

    QChart * chart = new QChart();
    // chart -> legend() -> hide();
    chart -> setTitle("QPieSeries");
    chart -> addSeries(pieSeries);
    chart -> setTheme(QChart::ChartTheme::ChartThemeDark);//设置背景的颜色
    chart -> setVisible(true);

    QChartView * chartView = new QChartView(chart);
    chartView -> setRenderHint(QPainter::Antialiasing); //启用抗锯齿效果
    chartView -> setVisible(true);


    if(this -> chartIndex == 0) {
        ui -> gridLayout -> addWidget(chartView,1,0);
    }

    this -> m_charts . append(chartView);
    return chartView;
}

QChartView *  ChartTheme::chartQBarMaxMinSeries()
{
    QStackedBarSeries * stackSeries = new QStackedBarSeries();

    QBarSet * minValue = new QBarSet("minValue");
    minValue -> append(-10);
    minValue -> append(-20);
    minValue -> append(-30);
    minValue -> append(-40);
    minValue -> append(-50);

    stackSeries -> append(minValue);

    QBarSet * maxValue = new QBarSet("maxValue");
    maxValue -> append(10);
    maxValue -> append(20);
    maxValue -> append(30);
    maxValue -> append(40);
    maxValue -> append(50);

    stackSeries -> append(maxValue);


    QChart * chart = new QChart();
    chart -> addSeries(stackSeries);
    chart -> legend() -> setVisible(true);
    chart -> legend() -> setAlignment(Qt::AlignBottom);
    chart -> setTitle("Value Of The Week");
    chart -> setTheme(QChart::ChartTheme::ChartThemeDark);
    chart -> setAnimationOptions(QChart::SeriesAnimations);

    QStringList days;
    days.append("Monday");
    days.append("Tuesday");
    days.append("Wednesday");
    days.append("Thursday");
    days.append("Friday");
    //自定义坐标轴
    QBarCategoryAxis * axisX = new QBarCategoryAxis();
    axisX -> append(days);
    axisX -> setTitleText("Week");
    chart -> addAxis(axisX,Qt::AlignBottom);
    stackSeries -> attachAxis(axisX);

    QValueAxis * axisY = new QValueAxis();
    axisY -> setRange(-60,60);
    axisY -> setTitleText("Values");
    chart -> addAxis(axisY,Qt::AlignLeft);
    stackSeries -> attachAxis(axisY);

    QChartView * chartView = new QChartView(chart);
    chartView -> setRenderHint(QPainter::Antialiasing);
    chartView -> setVisible(true);


    if(this -> chartIndex == 0) {
        ui -> gridLayout -> addWidget(chartView,1,1);
    }

    this -> m_charts . append(chartView);
    return chartView;
}



QChartView *  ChartTheme::chartQCandleSeries()
{
    QCandlestickSeries * series = new QCandlestickSeries();
    series -> setName("Bitcon Jan-May");
    series -> setIncreasingColor(QColor(Qt::green));
    series -> setDecreasingColor(QColor(Qt::cyan));

    QStringList Catagory;
    Catagory.append("Jan");
    series -> append(SetCandleSet(100.3,120.4,95.23,105.10));
    Catagory.append("Feb");
    series -> append(SetCandleSet(111.3,120.4,80.23,105.10));
    Catagory.append("March");
    series -> append(SetCandleSet(97.3,120.4,69.23,105.10));
    Catagory.append("April");
    series -> append(SetCandleSet(100.3,120.4,69.23,105.10));
    Catagory.append("May");
    series -> append(SetCandleSet(103.3,120.4,70.23,105.10));

    QChart * chart = new QChart();
    chart -> addSeries(series);
    chart -> setTitle("Bicon Data Of From Jan To May");
    chart -> setAnimationOptions(QChart::SeriesAnimations);
    chart -> createDefaultAxes();

    QBarCategoryAxis * axisX = qobject_cast<QBarCategoryAxis*>(chart -> axes(Qt::Horizontal).at(0));
    axisX -> setCategories(Catagory);

    QValueAxis * axisY = qobject_cast<QValueAxis*>(chart -> axes(Qt::Vertical).at(0));
    axisY -> setMax(axisY -> max() * 1.02);
    axisY -> setMin(axisY -> min() * 0.98);

    chart -> legend() -> setAlignment(Qt::AlignTop);
    chart -> legend() -> setVisible(true);
    chart -> setVisible(true);

    QChartView * chartView = new QChartView(chart);
    chartView -> setVisible(true);
    chartView -> setRenderHint(QPainter::Antialiasing);


    if(this -> chartIndex == 0) {
        ui -> gridLayout -> addWidget(chartView,1,2);
    }

    this -> m_charts . append(chartView);
    return chartView;
}

QChartView *ChartTheme::chartQAreaSeries()
{
    //使用 QLineSeries 对象来创建 QAreaSeries 对象。如果提供了两个 QLineSeries 对象，
    //则第一个对象定义上边界，第二个对象定义下边界。如果只提供一个 QLineSeries 对象，则默认使用绘图区域的底部作为下边界。
    QLineSeries *series1 = new QLineSeries();
    series1->append(0, 6);
    series1->append(2, 4);
    series1->append(3, 8);
    series1->append(7, 4);
    series1->append(10, 5);

    // 如果需要下边界，可以创建第二个 QLineSeries
    QLineSeries *series2 = new QLineSeries();
    series2->append(0, 2);
    series2->append(2, 1);
    series2->append(3, 3);
    series2->append(7, 1);
    series2->append(10, 2);

    QAreaSeries *areaSeries = new QAreaSeries(series1, series2); // 如果有下边界
    // 或者
    // QAreaSeries *areaSeries = new QAreaSeries(series1); // 如果没有下边界，使用默认下边界
    areaSeries->setName("Area Example");

    QChart * chart = new QChart();
    chart -> addSeries(areaSeries);
    chart -> setTitle("Area Example");
    chart -> setAnimationOptions(QChart::SeriesAnimations);

    QValueAxis *axisX = new QValueAxis();
    axisX->setRange(0, 10);
    axisX->setLabelFormat("%i");
    chart->addAxis(axisX, Qt::AlignBottom);
    areaSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 10);
    axisY->setLabelFormat("%.1f");
    chart->addAxis(axisY, Qt::AlignLeft);
    areaSeries->attachAxis(axisY);

    QChartView * chartView = new QChartView(chart);
    chartView -> setVisible(true);
    chartView -> setRenderHint(QPainter::Antialiasing);


    if(this -> chartIndex == 0) {
        ui -> gridLayout -> addWidget(chartView,1,3);
    }

    this -> m_charts . append(chartView);
    return chartView;
}

QCandlestickSet *ChartTheme::SetCandleSet(double open, double high, double low, double close)
{
    QCandlestickSet * set = new QCandlestickSet();
    set -> setOpen(open);
    set -> setHigh(high);
    set -> setLow(low);
    set -> setClose(close);
    return set;
}

void ChartTheme::on_pushButton_clicked()
{
    this -> close();
}

void ChartTheme::selectCharts(int index)
{
    this -> chartIndex = index;
    switch(index){
    case 0:
        this -> removeLayout();

        this -> chartQLineSeries();
        this -> chartQSPlineSeries();
        this -> chartQBarSeries();
        this -> chartQScatterSeries();

        this -> chartQPieSeries();
        this -> chartQBarMaxMinSeries();
        this -> chartQCandleSeries();
        this -> chartQAreaSeries();
        break;
    case 1:
        this -> removeLayout();
        ui -> gridLayout -> addWidget(this -> chartQLineSeries(),0,0);
        break;
    case 2:
        this -> removeLayout();
        ui -> gridLayout -> addWidget(this -> chartQSPlineSeries(),0,0);
        break;
    case 3:
        this -> removeLayout();
        ui -> gridLayout -> addWidget(this -> chartQBarSeries(),0,0);
        break;
    case 4:
        this -> removeLayout();
        ui -> gridLayout -> addWidget(this -> chartQPieSeries(),0,0);
        break;
    case 5:
        this -> removeLayout();
        ui -> gridLayout -> addWidget(this -> chartQCandleSeries(),0,0);
        break;
    case 6:
        this -> removeLayout();
        ui -> gridLayout -> addWidget(this -> chartQBarMaxMinSeries(),0,0);
        break;
    case 7:
        this -> removeLayout();
        ui -> gridLayout -> addWidget(this -> chartQScatterSeries(),0,0);
        break;
    case 8:
        this -> removeLayout();
        ui -> gridLayout -> addWidget(this -> chartQAreaSeries(),0,0);
        break;
    default:
        break;
    }
}


void ChartTheme::updateUI()
{
    QChart::ChartTheme theme = static_cast<QChart::ChartTheme>(ui->themeComboBox->itemData(this -> themeIndex).toInt());
    const auto charts = m_charts;
    if (!m_charts.isEmpty() && m_charts.at(0)->chart()->theme() != theme) {
        for (QChartView *chartView : charts) {
            chartView->chart()->setTheme(theme);
        }

        // Set palette colors based on selected theme 设置整个窗口的颜色
        QPalette pal = window()->palette();
        if (theme == QChart::ChartThemeLight) {
            pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
            pal.setColor(QPalette::WindowText, QRgb(0x404044));

        } else if (theme == QChart::ChartThemeDark) {
            pal.setColor(QPalette::Window, QRgb(0x121218));
            pal.setColor(QPalette::WindowText, QRgb(0xd6d6d6));
        } else if (theme == QChart::ChartThemeBlueCerulean) {
            pal.setColor(QPalette::Window, QRgb(0x40434a));
            pal.setColor(QPalette::WindowText, QRgb(0xd6d6d6));
        } else if (theme == QChart::ChartThemeBrownSand) {
            pal.setColor(QPalette::Window, QRgb(0x9e8965));
            pal.setColor(QPalette::WindowText, QRgb(0x404044));
        } else if (theme == QChart::ChartThemeBlueNcs) {
            pal.setColor(QPalette::Window, QRgb(0x018bba));
            pal.setColor(QPalette::WindowText, QRgb(0x404044));
        } else if (theme == QChart::ChartThemeHighContrast) {
            pal.setColor(QPalette::Window, QRgb(0xffab03));
            pal.setColor(QPalette::WindowText, QRgb(0x181818));
        } else if (theme == QChart::ChartThemeBlueIcy) {
            pal.setColor(QPalette::Window, QRgb(0xcee7f0));
            pal.setColor(QPalette::WindowText, QRgb(0x404044));
        } else {
            pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
            pal.setColor(QPalette::WindowText, QRgb(0x404044));
        }
        window()->setPalette(pal);
    }

    // Update antialiasing
    for (QChartView *chart : charts){
        chart->setRenderHint(QPainter::Antialiasing);
    }

    // Update animation options
    QChart::AnimationOptions options(ui->animatedComboBox->itemData(this -> AnimationIndex).toInt());
    if (!m_charts.isEmpty() && m_charts.at(0)->chart()->animationOptions() != options) {
        for (QChartView *chartView : charts)
            chartView->chart()->setAnimationOptions(options);
    }

    // Update legend alignment
    Qt::Alignment alignment(ui->legendComboBox->itemData(this -> legendIndex).toInt());

    if (!alignment) {
        for (QChartView *chartView : charts)
            chartView->chart()->legend()->hide();
    } else {
        for (QChartView *chartView : charts) {
            chartView->chart()->legend()->setAlignment(alignment);
            chartView->chart()->legend()->show();
        }
    }
}
//移除当前布局中的控件
void ChartTheme::removeLayout()
{
    qDebug()<<"row number: "<<ui -> gridLayout -> rowCount();
    qDebug()<<"col number: "<<ui -> gridLayout -> columnCount();

    int row = ui -> gridLayout -> rowCount();
    int col = ui -> gridLayout -> columnCount();
    for(int i = 0 ; i < row ; i ++){
        for(int j = 0 ; j < col ; j++){
            qDebug()<<"i = "<<i;
            qDebug()<<"j = "<<j;
            QLayoutItem * widgetItem = ui -> gridLayout -> itemAtPosition(i,j);
            if(widgetItem){
                QWidget * widget = widgetItem -> widget();
                delete widget;
                // widget -> setParent(nullptr);
                // ui -> gridLayout -> itemAtPosition(i,j) -> widget() -> setParent(nullptr);
            }
        }
    }
    this -> m_charts.clear();
}

