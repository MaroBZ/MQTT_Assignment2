#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QDebug>

MainWindow *handle;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->ui->comboBox->addItem("ee513/Data");
    this->ui->comboBox->addItem("ee513/TEST");

    this->setWindowTitle("EE513 Assignment 2");

    this->ui->customPlot->addGraph();
    this->ui->customPlot->yAxis->setLabel("CPU Temperature (°C)");
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    this->ui->customPlot->xAxis->setTicker(timeTicker);
    this->ui->customPlot->yAxis->setRange(20, 60);
    this->ui->customPlot->replot();

    this->ui->sensorPlot->addGraph();
    this->ui->sensorPlot->graph(0)->setName("Pitch");
    this->ui->sensorPlot->addGraph();
    this->ui->sensorPlot->graph(1)->setName("Roll");
    this->ui->sensorPlot->graph(1)->setPen(QPen(Qt::red));
    this->ui->sensorPlot->xAxis->setLabel("Time");
    this->ui->sensorPlot->yAxis->setLabel("Degrees");
    this->ui->sensorPlot->xAxis->setTicker(timeTicker);
    this->ui->sensorPlot->yAxis->setRange(-180, 180); 
    this->ui->sensorPlot->legend->setVisible(true);
    this->ui->sensorPlot->replot();

    QObject::connect(this, SIGNAL(messageSignal(QString)),
                     this, SLOT(on_MQTTmessage(QString)));
    QObject::connect(ui->comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(topicSelect(QString)));
    ::handle = this;

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_connectButton_clicked()
{
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    if (MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)==0){
        ui->outputText->appendPlainText(QString("Callbacks set correctly"));
    }
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        ui->outputText->appendPlainText(QString("Failed to connect, return code %1").arg(rc));
    }

    // Unsubscribe from the previous topic if it exists
    if (!TOPIC.isEmpty()) {
        MQTTClient_unsubscribe(client, TOPIC.toUtf8().constData());
        ui->outputText->appendPlainText(QString("Unsubscribed from topic %1").arg(TOPIC));
    }

    // Get the currently selected topic from the ComboBox
    TOPIC = ui->comboBox->currentText();

    // Subscribe to the new topic
    rc = MQTTClient_subscribe(client, TOPIC.toUtf8().constData(), QOS);
    if (rc == MQTTCLIENT_SUCCESS) {
        ui->outputText->appendPlainText(QString("Subscribed successfully to topic %1").arg(TOPIC));
    } else {
        ui->outputText->appendPlainText(QString("Failed to subscribe, return code %1").arg(rc));
    }
}

void MainWindow::topicSelect(QString topic)
{
    this->TOPIC = topic;
}

void delivered(void *context, MQTTClient_deliveryToken dt) {
    (void)context;
    // Please don't modify the Window UI from here
    qDebug() << "Message delivery confirmed";
    handle->deliveredtoken = dt;
}

/* This is a callback function and is essentially another thread. Do not modify the
 * main window UI from here as it will cause problems. Please see the Slot method that
 * is directly below this function. To ensure that this method is thread safe I had to
 * get it to emit a signal which is received by the slot method below */
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    (void)context; (void)topicLen;
    qDebug() << "Message arrived (topic is " << topicName << ")";
    qDebug() << "Message payload length is " << message->payloadlen;
    QString payload;
    payload.sprintf("%s", (char *) message->payload).truncate(message->payloadlen);
    emit handle->messageSignal(payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

/** This is the slot method. Do all of your message received work here. It is also safe
 * to call other methods on the object from this point in the code */
void MainWindow::on_MQTTmessage(QString payload){
    ui->outputText->appendPlainText(payload);
    ui->outputText->ensureCursorVisible();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(payload.toUtf8());
    QJsonObject jsonObject = jsonDoc.object();

    double cpuTemp = jsonObject["d"].toObject()["CPU_TEMP"].toDouble();
    qDebug() << "CPU Temperature: " << cpuTemp;
    double pitch = jsonObject["d"].toObject()["ADXL345"].toObject()["Pitch"].toDouble();
    double roll = jsonObject["d"].toObject()["ADXL345"].toObject()["Roll"].toDouble();


    updateTempPlot(QTime::currentTime().msecsSinceStartOfDay()/1000.0, cpuTemp);
    updateSensorPlot(QTime::currentTime().msecsSinceStartOfDay()/1000.0, pitch, roll);

}

void MainWindow::updateTempPlot(double x, double y) {
    this->ui->customPlot->graph(0)->addData(x, y);
    this->ui->customPlot->xAxis->rescale(true);
    this->ui->customPlot->replot();
}

void MainWindow::updateSensorPlot(double x, double pitch, double roll) {
    this->ui->sensorPlot->graph(0)->addData(x, pitch);
    this->ui->sensorPlot->graph(1)->addData(x, roll);

    // rescale to fit the new data
    this->ui->sensorPlot->xAxis->rescale(true);

    this->ui->sensorPlot->replot();
}

void connlost(void *context, char *cause) {
    (void)context; (void)*cause;
    // Please don't modify the Window UI from here
    qDebug() << "Connection Lost" << endl;
}

void MainWindow::on_disconnectButton_clicked()
{
    qDebug() << "Disconnecting from the broker" << endl;
    MQTTClient_disconnect(client, 10000);
    //MQTTClient_destroy(&client);
}
