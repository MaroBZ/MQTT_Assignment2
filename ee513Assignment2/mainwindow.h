#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include "MQTTClient.h"

#define ADDRESS     "tcp://127.0.0.1:1883"
#define CLIENTID    "rpi2"
#define AUTHMETHOD  "maro"
#define AUTHTOKEN   "admin"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void on_connectButton_clicked();
    void on_disconnectButton_clicked();
    void on_MQTTmessage(QString message);
    void topicSelect(QString topic);

signals:
    void messageSignal(QString message);

private:
    QString TOPIC;
    Ui::MainWindow *ui;
    void updateTempPlot(double x, double y);
    void updateSensorPlot(double x, double pitch, double roll);
    MQTTClient client;
    volatile MQTTClient_deliveryToken deliveredtoken;

    friend void delivered(void *context, MQTTClient_deliveryToken dt);
    friend int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
    friend void connlost(void *context, char *cause);
};

void delivered(void *context, MQTTClient_deliveryToken dt);
int  msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
void connlost(void *context, char *cause);

#endif // MAINWINDOW_H

