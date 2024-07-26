#ifndef CHARTTHEME_H
#define CHARTTHEME_H

#include <QMainWindow>

#include <QtCharts/QtCharts>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>


QT_BEGIN_NAMESPACE
namespace Ui {
class ChartTheme;
}
QT_END_NAMESPACE

class ChartTheme : public QMainWindow
{
    Q_OBJECT

public:
    ChartTheme(QWidget *parent = nullptr);
    ~ChartTheme();

    QChartView *  chartQLineSeries();
    QChartView *  chartQSPlineSeries();
    QChartView *  chartQBarSeries();
    QChartView *  chartQPieSeries();
    QChartView *  chartQBarMaxMinSeries();
    QChartView *  chartQScatterSeries();
    QChartView *  chartQAreaSeries();

    void populateLegendBox();
    void populateAnimationBox();
    void populateThemeBox();
    void populateChartBox();

    QChartView *  chartQCandleSeries();
    QCandlestickSet * SetCandleSet(double open,double high,double low,double close);

    void updateUI();
    void removeLayout();

private slots:
    void on_pushButton_clicked();
    void selectCharts(int index);

private:
    Ui::ChartTheme *ui;

    int themeIndex = 0;
    int AnimationIndex = 0;
    int legendIndex = 0;
    int chartIndex = 0;

    QList<QChartView *> m_charts;
};
#endif // CHARTTHEME_H
