#include "sysmon.h"
#include <QGridLayout>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QCoreApplication>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdio>

QT_USE_NAMESPACE

SysmonV3::SysmonV3(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("System Monitor");
    resize(1000, 800);

    nvmlEnabled = (nvmlInit() == NVML_SUCCESS);

    // Discover correct CPU temperature zone
    for (int i = 0; i < 10; ++i) {
        std::ifstream typeFile("/sys/class/thermal/thermal_zone" + std::to_string(i) + "/type");
        std::string type;
        if (typeFile >> type && type == "x86_pkg_temp") {
            cpuTempPath = QString("/sys/class/thermal/thermal_zone%1/temp").arg(i);
            break;
        }
    }
    if (cpuTempPath.isEmpty()) cpuTempPath = "/sys/class/thermal/thermal_zone0/temp";

    setupUI();
    setupTray();

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &SysmonV3::updateStats);
    updateTimer->start(100);
}

SysmonV3::~SysmonV3() {
    if (nvmlEnabled) nvmlShutdown();
}

void SysmonV3::closeEvent(QCloseEvent *event) {
    if (trayIcon->isVisible()) {
        hide(); // Minimize to tray instead of closing
        event->ignore();
    }
}

void SysmonV3::setupUI() {
    auto *central = new QWidget(this);
    auto *layout = new QGridLayout(central);

    auto createChart = [&](QLineSeries** series, QChartView** view, QString title, QColor color, QLineSeries** series2 = nullptr, QColor color2 = Qt::red) {
        *series = new QLineSeries();
        auto pen = (*series)->pen();
        pen.setWidth(2);
        pen.setColor(color);
        (*series)->setPen(pen);

        QChart *chart = new QChart();
        chart->addSeries(*series);

        if (series2) {
            *series2 = new QLineSeries();
            auto pen2 = (*series2)->pen();
            pen2.setWidth(1);
            pen2.setColor(color2);
            pen2.setStyle(Qt::DashLine);
            (*series2)->setPen(pen2);
            chart->addSeries(*series2);
        }

        auto axisX = new QDateTimeAxis();
        axisX->setFormat("HH:mm:ss");
        axisX->setLabelsColor(Qt::gray);
        axisX->setGridLineColor(QColor(60, 60, 60));
        chart->addAxis(axisX, Qt::AlignBottom);
        (*series)->attachAxis(axisX);
        if (series2) (*series2)->attachAxis(axisX);

        auto axisY = new QValueAxis();
        axisY->setRange(0, 100);
        axisY->setLabelsColor(Qt::gray);
        axisY->setGridLineColor(QColor(60, 60, 60));
        chart->addAxis(axisY, Qt::AlignLeft);
        (*series)->attachAxis(axisY);
        if (series2) (*series2)->attachAxis(axisY);

        chart->setBackgroundBrush(QBrush(QColor(25, 25, 25)));
        chart->setTitleBrush(QBrush(Qt::white));
        chart->setTitle(title);
        chart->legend()->hide();

        if (series2) {
            chart->legend()->show();
            chart->legend()->setAlignment(Qt::AlignBottom);
            chart->legend()->setLabelColor(Qt::white);
            (*series)->setName("Usage/Load");
            (*series2)->setName("Temp (°C)");
        }

        *view = new QChartView(chart);
        (*view)->setRenderHint(QPainter::Antialiasing);
        (*view)->setStyleSheet("background-color: #191919; border: 1px solid #333;");
    };

    createChart(&cpuSeries, &cpuView, "CPU Usage & Temp", Qt::cyan, &cpuTempSeries, Qt::red);
    createChart(&ramSeries, &ramView, "RAM Usage (%)", Qt::green);
    createChart(&gpuSeries, &gpuView, "GPU Load & Temp", Qt::yellow, &gpuTempSeries, Qt::red);
    createChart(&vramSeries, &vramView, "VRAM Usage (%)", Qt::magenta);
    createChart(&netRxSeries, &netView, "Network (KB/s)", Qt::blue, &netTxSeries, Qt::white);
    if (netRxSeries && netTxSeries) {
        netRxSeries->setName("RX");
        netTxSeries->setName("TX");
    }

    processTable = new QTableWidget(10, 3);
    processTable->setHorizontalHeaderLabels({"PID", "Name", "CPU%"});
    processTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    processTable->setStyleSheet("QTableWidget { background-color: #111; color: #EEE; gridline-color: #333; border: 1px solid #333; font-family: monospace; }"
                               "QHeaderView::section { background-color: #222; color: #AAA; border: 1px solid #333; }");
    processTable->verticalHeader()->hide();
    processTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(cpuView, 0, 0);
    layout->addWidget(ramView, 0, 1);
    layout->addWidget(gpuView, 1, 0);
    layout->addWidget(vramView, 1, 1);
    layout->addWidget(netView, 2, 0);
    layout->addWidget(processTable, 2, 1);

    footerLabel = new QLabel("Reading Sensors...");
    footerLabel->setStyleSheet("font-size: 13px; font-family: monospace; font-weight: bold; padding: 10px; color: #00FF00; background: #111;");
    layout->addWidget(footerLabel, 3, 0, 1, 2);

    setCentralWidget(central);
    setStyleSheet("QMainWindow { background-color: #0A0A0A; }");
}

void SysmonV3::setupTray() {
    trayIcon = new QSystemTrayIcon(this);
    updateTrayIcon(0, 0);

    QMenu *menu = new QMenu(this);
    menu->addAction("Show Window", this, &SysmonV3::showNormal);
    menu->addSeparator();
    menu->addAction("Exit", qApp, &QCoreApplication::quit);

    trayIcon->setContextMenu(menu);
    trayIcon->show();
}

void SysmonV3::updateTrayIcon(int cpu, int gpu) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::TextAntialiasing);

    painter.setPen(Qt::cyan); // Match CPU graph color
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(QRect(0, 0, 32, 16), Qt::AlignCenter, QString("%1").arg(cpu));

    painter.setPen(Qt::yellow); // Match GPU graph color
    painter.drawText(QRect(0, 16, 32, 16), Qt::AlignCenter, QString("%1").arg(gpu));

    painter.end();
    trayIcon->setIcon(QIcon(pixmap));
}

double SysmonV3::getCPUUsage() {
    static long long lastTotal = 0, lastIdle = 0;
    std::ifstream file("/proc/stat");
    std::string label;
    long long u, n, s, i, io, irq, si;
    if (!(file >> label >> u >> n >> s >> i >> io >> irq >> si)) return 0;
    long long total = u + n + s + i + io + irq + si;
    long long idle = i + io;
    double usage = (lastTotal > 0) ? 100.0 * (1.0 - (double)(idle - lastIdle) / (total - lastTotal)) : 0;
    lastTotal = total; lastIdle = idle;
    return usage;
}

double SysmonV3::getRAMUsage() {
    std::ifstream file("/proc/meminfo");
    long total = 1, available = 0;
    std::string line, key;
    long value;
    while (file >> key >> value >> line) {
        if (key == "MemTotal:") total = value;
        if (key == "MemAvailable:") available = value;
    }
    return 100.0 * (1.0 - (double)available / total);
}

int SysmonV3::getCPUTemp() {
    std::ifstream file(cpuTempPath.toStdString());
    int temp = 0;
    if (file >> temp) return temp / 1000;
    return 0;
}

SysmonV3::GPUStats SysmonV3::getGPUStats() {
    GPUStats stats = {0, 0, 0};
    if (!nvmlEnabled) return stats;

    nvmlDevice_t device;
    if (nvmlDeviceGetHandleByIndex(0, &device) == NVML_SUCCESS) {
        nvmlUtilization_t util;
        if (nvmlDeviceGetUtilizationRates(device, &util) == NVML_SUCCESS) stats.load = util.gpu;
        
        nvmlMemory_t mem;
        if (nvmlDeviceGetMemoryInfo(device, &mem) == NVML_SUCCESS) {
            stats.vram = (int)(100.0 * mem.used / mem.total);
        }
        
        unsigned int temp;
        if (nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS) {
            stats.temp = (int)temp;
        }
    }
    return stats;
}

SysmonV3::NetStats SysmonV3::getNetworkTraffic() {
    std::ifstream file("/proc/net/dev");
    std::string line;
    qint64 rx = 0, tx = 0;
    while (std::getline(file, line)) {
        if (line.find(":") != std::string::npos && line.find("lo:") == std::string::npos) {
            std::string iface, r_bytes, r_pkts, r_errs, r_drop, r_fifo, r_frame, r_compr, r_multi, t_bytes;
            std::stringstream ss(line);
            ss >> iface >> r_bytes >> r_pkts >> r_errs >> r_drop >> r_fifo >> r_frame >> r_compr >> r_multi >> t_bytes;
            rx += std::stoll(r_bytes);
            tx += std::stoll(t_bytes);
        }
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    double rxRate = 0, txRate = 0;
    if (lastNetTime > 0) {
        double dt = (now - lastNetTime) / 1000.0;
        rxRate = (rx - lastRx) / 1024.0 / dt;
        txRate = (tx - lastTx) / 1024.0 / dt;
    }
    lastRx = rx; lastTx = tx; lastNetTime = now;
    return {rxRate, txRate};
}

void SysmonV3::updateProcessTable() {
    // A simplified process list using ps command for robustness
    FILE* pipe = popen("ps -eo pid,pcpu,comm --sort=-pcpu | head -n 11", "r");
    if (!pipe) return;
    char buffer[128];
    int row = 0;
    fgets(buffer, 128, pipe); // Header
    while (fgets(buffer, 128, pipe) && row < 10) {
        char pid[16], cpu[16], comm[64];
        sscanf(buffer, "%s %s %s", pid, cpu, comm);
        processTable->setItem(row, 0, new QTableWidgetItem(pid));
        processTable->setItem(row, 1, new QTableWidgetItem(comm));
        processTable->setItem(row, 2, new QTableWidgetItem(cpu));
        row++;
    }
    pclose(pipe);
}

void SysmonV3::updateStats() {
    double cpu = getCPUUsage();
    double ram = getRAMUsage();
    int cTemp = getCPUTemp();
    GPUStats gStats = getGPUStats();
    NetStats nStats = getNetworkTraffic();
    updateProcessTable();

    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    auto updateLine = [&](QLineSeries* series, QChartView* view, double val, double maxRange = 100) {
        if (!series) return;
        series->append(now, val);
        if (series->count() > 600) series->remove(0); 

        auto axisX = qobject_cast<QDateTimeAxis*>(view->chart()->axes(Qt::Horizontal).first());
        if (axisX) {
            axisX->setRange(QDateTime::fromMSecsSinceEpoch(now - 60000), QDateTime::fromMSecsSinceEpoch(now));
        }
        if (maxRange > 100) {
            auto axisY = qobject_cast<QValueAxis*>(view->chart()->axes(Qt::Vertical).first());
            if (axisY && val > axisY->max()) axisY->setRange(0, val * 1.2);
        }
    };

    updateLine(cpuSeries, cpuView, cpu);
    updateLine(cpuTempSeries, cpuView, (double)cTemp);
    updateLine(ramSeries, ramView, ram);
    updateLine(gpuSeries, gpuView, (double)gStats.load);
    updateLine(gpuTempSeries, gpuView, (double)gStats.temp);
    updateLine(vramSeries, vramView, (double)gStats.vram);
    updateLine(netRxSeries, netView, nStats.rx, 1000);
    updateLine(netTxSeries, netView, nStats.tx, 1000);

    // Removed timeStep++ as we use real-time now

    footerLabel->setText(QString("CPU: %1% [%2°C] | RAM: %3% | GPU: %4% [%5°C] | VRAM: %6% | Net: %7/%8 KB/s")
                         .arg((int)cpu).arg(cTemp).arg((int)ram)
                         .arg(gStats.load).arg(gStats.temp).arg(gStats.vram)
                         .arg((int)nStats.rx).arg((int)nStats.tx));

    updateTrayIcon((int)cpu, gStats.load);
}