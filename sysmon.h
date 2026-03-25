#ifndef SYSMON_H
#define SYSMON_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QLabel>
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QTableWidget>
#include <QHeaderView>
#include <nvml.h>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QDateTime>

QT_USE_NAMESPACE

class SysmonV3 : public QMainWindow {
    Q_OBJECT

public:
    explicit SysmonV3(QWidget *parent = nullptr);
    ~SysmonV3();

protected:
    void closeEvent(QCloseEvent *event) override; // Intercept "X" button

private slots:
    void updateStats();

private:
    QLineSeries *cpuSeries;
    QLineSeries *cpuTempSeries;
    QLineSeries *ramSeries;
    QLineSeries *gpuSeries;
    QLineSeries *gpuTempSeries;
    QLineSeries *vramSeries;
    QLineSeries *netRxSeries;
    QLineSeries *netTxSeries;

    QChartView *cpuView;
    QChartView *ramView;
    QChartView *gpuView;
    QChartView *vramView;
    QChartView *netView;
    QTableWidget *processTable;

    QSystemTrayIcon *trayIcon;
    QTimer *updateTimer;
    QLabel *footerLabel;
    bool nvmlEnabled = false;
    QString cpuTempPath;

    double timeStep = 0;

    void setupUI();
    void setupTray();
    void updateTrayIcon(int cpu, int gpu);

    double getCPUUsage();
    double getRAMUsage();
    int getCPUTemp();

    struct GPUStats {
        int load;
        int vram;
        int temp;
    };
    GPUStats getGPUStats();

    struct NetStats {
        double rx;
        double tx;
    };
    NetStats getNetworkTraffic();
    qint64 lastRx = 0, lastTx = 0;
    qint64 lastNetTime = 0;

    void updateProcessTable();
};

#endif