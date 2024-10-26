#include "dialog.h"
#include "ui_dialog.h"
#include "savedata.h"
#include "param.h"
#include <QSerialPortInfo>
#include <QList>
#include <QDebug>
#include <QLineEdit>
#include <QTimer>
#include <QSize>
#include <QByteArray>
#include <QDateTime>
#include <QFileDialog>
#include <QCoreApplication>
#include <QDir>
#include <QStringList>
#include <QPushButton>
#include <QString>
#include <QPalette>
#include <QStringList>
#include <QVariantMap>
#include <stdio.h>
#include <string.h>
#include <QtSerialPort>

#define READ_BUFFER_SIZE 200

union
{
    uint32_t uint;
    float floatVal;
}myUnion;

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    Qt::WindowFlags windowFlag = Qt::Dialog;
    windowFlag |= Qt::WindowMinimizeButtonHint;
    windowFlag |= Qt::WindowMaximizeButtonHint;
    windowFlag |= Qt::WindowCloseButtonHint;
    setWindowFlags(windowFlag);

    this->setWindowIcon(QIcon(":/res/log2.png"));
    this->setWindowTitle("V0103_上位机");
    ui->label_log->setStyleSheet("QLabel {" "background-image: url(:/res/ql.png);" "border: none;" "}");
    ui->label_log->show();

    allInit();

    // 定时器1 用于更新串口号
    QTimer * timer1 = new QTimer(this);
    connect(timer1, &QTimer::timeout,[=](){
        if (mIsOpen)
            return;
        else
            UpdatePort();
    });
    timer1->start(1000);

    // 定时器 保存数据
    mSaveTimer = new QTimer(this);
    mSerialPort = new QSerialPort(this);
    // 定时器 用于定时查询
    periodicQueryTimer = new QTimer(this);
    connect(periodicQueryTimer, &QTimer::timeout, this, &Dialog::periodicQuery);
    periodicQueryTimer->start(PERIODIC_QUERY_TIME);
    // 智能识别当前系统有效端口号
    QStringList newPortStringList;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo & info : infos)
    {
        newPortStringList += info.portName();
    }
    // 更新串口号
    if (newPortStringList.size() != oldPortStringList.size())
    {
        oldPortStringList = newPortStringList;
        ui->CBoxSerialPort->clear();
        ui->CBoxSerialPort->addItems(oldPortStringList);
    }

    //connect(mSerialPort, SIGNAL(readyRead()), this, SLOT(RecData()));
    connect(mSerialPort, &QSerialPort::readyRead, this, &Dialog::RecData);

    connect(ui->pushButton_saveStart, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            if (mSave_Flag)
            {
                ui->pushButton_saveStart->setText("开始保存");
                show_PromptInformation("停止保存！");
                mSaveTimer->stop();
                ui->pushButton_saveStart->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
            }
            else
            {
                int saveTime = ui->lineEdit_saveTime->text().toInt();
                if (saveTime <= 0)
                {
                    show_PromptInformation_Red("请检查储存周期设置是否正确！");
                    return ;
                }
                ui->pushButton_saveStart->setText("停止保存");
                show_PromptInformation("开始保存！");
                mSaveTimer->start(saveTime*1000);
                ui->pushButton_saveStart->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            mSave_Flag = !mSave_Flag;
        }

    });

    connect(ui->pushButton_setSavePath, &QPushButton::clicked, [=](){
        mSavePath = QFileDialog::getExistingDirectory(this, "选择文件夹", QDir::currentPath());
        show_PromptInformation("文件保存路径设置成功！");
    });

    connect(ui->pushButton_setPathName, &QPushButton::clicked, [=](){
        mPathName = ui->lineEdit_pathName->text();
        show_PromptInformation("文件名设置成功！");
    });

    connect(mSaveTimer, &QTimer::timeout, this, &Dialog::SaveData);

    connect(ui->pushButton_write, &QPushButton::clicked, [=](){
        uint8_t n = 3;
        while(n--)
        {
            SetParsing();
        }

    });
    connect(ui->pushButton_read, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            uint8_t buf[4] = { 0 };
            buf[0] = 0X01;
            sendToKz(CMD_NORMALPARAM_READ, buf, 1);
        }
    });

    connect(ui->pushButton_init, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            uint8_t buf[4] = { 0 };
            buf[0] = 0X02;
            sendToKz(CMD_NORMALPARAM_READ, buf, 1);
        }
    });

    connect(ui->pushButton_weixiu, &QPushButton::clicked, [=](){
        if (mWeixiu_Flag)
        {
            uint32_t i = 0;
            mSendBuf[i++] = MODE_MAINTAIN;
            mSendBuf[i++] = OPEN;
            sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
            ui->pushButton_weixiu->setText("维修模式开");
            show_PromptInformation("维修模式：关！！！");
            ui->pushButton_weixiu->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
        }
        else
        {
            uint32_t i = 0;
            mSendBuf[i++] = MODE_MAINTAIN;
            mSendBuf[i++] = CLOSE;
            sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
            ui->pushButton_weixiu->setText("维修模式关");
            show_PromptInformation("维修模式：开！！！");
            ui->pushButton_weixiu->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
        }
        mWeixiu_Flag = !mWeixiu_Flag;
    });

    connect(ui->pushButton_gfjck, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            if (mGuFen_Flag)
            {
                uint32_t i = 0;
                mSendBuf[i++] = DEVICE_BLOWER;
                mSendBuf[i++] = OPEN;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_gfjck->setText("关闭");
                show_PromptInformation("鼓风机常开：开！！！");
                ui->pushButton_gfjck->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint32_t i = 0;
                mSendBuf[i++] = DEVICE_BLOWER;
                mSendBuf[i++] = CLOSE;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_gfjck->setText("打开");
                show_PromptInformation("鼓风机常开：关！！！");
                ui->pushButton_gfjck->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
            }
            mGuFen_Flag = !mGuFen_Flag;
        }

    });

    connect(ui->pushButton_fsck, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            if (mFenSan_Flag)
            {
                uint32_t i = 0;
                mSendBuf[i++] = DEVICE_FAN;
                mSendBuf[i++] = OPEN;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_fsck->setText("关闭");
                show_PromptInformation("冷却风扇常开：开！！！");
                ui->pushButton_fsck->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint32_t i = 0;
                mSendBuf[i++] = DEVICE_FAN;
                mSendBuf[i++] = CLOSE;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_fsck->setText("打开");
                show_PromptInformation("冷却风扇常开：关！！！");
                ui->pushButton_fsck->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
            }
            mFenSan_Flag = !mFenSan_Flag;
        }

    });

    connect(ui->pushButton_qingin, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            if (mQingIn_Flag)
            {
                uint32_t i = 0;
                mSendBuf[i++] = DEVICE_VALVEIN;
                mSendBuf[i++] = OPEN;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_qingin->setText("关闭");
                show_PromptInformation("氢气进口：开！！！");
                ui->pushButton_qingin->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint32_t i = 0;
                mSendBuf[i++] = DEVICE_VALVEIN;
                mSendBuf[i++] = CLOSE;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_qingin->setText("打开");
                show_PromptInformation("氢气进口：关！！！");
                ui->pushButton_qingin->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
            }
            mQingIn_Flag = !mQingIn_Flag;
        }

    });

    connect(ui->pushButton_qingin_2, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            if (mQingIn_Flag2)
            {
                uint32_t i = 0;
                uint32_t tempuint;
                float tempfl;

                mSendBuf[i++] = 0X14;
                mSendBuf[i++] = ui->lineEdit_cycleTIM->text().toUInt();

                tempfl = ui->lineEdit_Exhausttime_Y1->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                // 燃料电池温度上限
                tempfl = ui->lineEdit_WXranliaoTemperMAX->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                // 燃料电池运行目标温度
                tempfl = ui->lineEdit_WXranliaoTargetTemper->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                mSendBuf[i++] = ui->lineEdit_GFJ_PWM->text().toUInt();

                sendToKz(CMD_RM_SETTING, mSendBuf, i);
                ui->pushButton_qingin_2->setText("关闭");
                show_PromptInformation("进气阀常开：开！");
                ui->pushButton_qingin_2->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint32_t i = 0;
                uint32_t tempuint;
                float tempfl;

                mSendBuf[i++] = 0X15;
                mSendBuf[i++] = ui->lineEdit_cycleTIM->text().toUInt();

                tempfl = ui->lineEdit_Exhausttime_Y1->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                // 燃料电池温度上限
                tempfl = ui->lineEdit_WXranliaoTemperMAX->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                // 燃料电池运行目标温度
                tempfl = ui->lineEdit_WXranliaoTargetTemper->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                mSendBuf[i++] = ui->lineEdit_GFJ_PWM->text().toUInt();

                sendToKz(CMD_RM_SETTING, mSendBuf, i);

                ui->pushButton_qingin_2->setText("打开");
                show_PromptInformation("进气阀常开：关！");
                ui->pushButton_qingin_2->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
            }
            mQingIn_Flag2 = !mQingIn_Flag2;
        }

    });

    connect(ui->pushButton_qingout, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            if (mQingOut_Flag)
            {
                uint32_t i = 0;
                mSendBuf[i++] = DEVICE_VALVEOUT;
                mSendBuf[i++] = OPEN;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_qingout->setText("关闭");
                show_PromptInformation("氢气出口：开！");
                ui->pushButton_qingout->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint32_t i = 0;
                mSendBuf[i++] = DEVICE_VALVEOUT;
                mSendBuf[i++] = CLOSE;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_qingout->setText("打开");
                show_PromptInformation("氢气出口：关！");
                ui->pushButton_qingout->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
            }
            mQingOut_Flag = !mQingOut_Flag;
        }

    });

    connect(ui->pushButton_qingout_2, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            if (mQingOut_Flag2)
            {
                uint32_t i = 0;
                uint32_t tempuint;
                float tempfl;

                mSendBuf[i++] = 0X16;
                mSendBuf[i++] = ui->lineEdit_cycleTIM->text().toUInt();

                tempfl = ui->lineEdit_Exhausttime_Y1->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                // 燃料电池温度上限
                tempfl = ui->lineEdit_WXranliaoTemperMAX->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                // 燃料电池运行目标温度
                tempfl = ui->lineEdit_WXranliaoTargetTemper->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                mSendBuf[i++] = ui->lineEdit_GFJ_PWM->text().toUInt();

                sendToKz(CMD_RM_SETTING, mSendBuf, i);

                ui->pushButton_qingout_2->setText("关闭");
                show_PromptInformation("排气阀常开：开！");
                ui->pushButton_qingout_2->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint32_t i = 0;
                uint32_t tempuint;
                float tempfl;

                mSendBuf[i++] = 0X17;
                mSendBuf[i++] = ui->lineEdit_cycleTIM->text().toUInt();

                tempfl = ui->lineEdit_Exhausttime_Y1->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                // 燃料电池温度上限
                tempfl = ui->lineEdit_WXranliaoTemperMAX->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                // 燃料电池运行目标温度
                tempfl = ui->lineEdit_WXranliaoTargetTemper->text().toFloat();
                tempuint = tempfl * 10;
                mSendBuf[i++] = (uint8_t)(tempuint);
                mSendBuf[i++] = (uint8_t)(tempuint >> 8);

                mSendBuf[i++] = ui->lineEdit_GFJ_PWM->text().toUInt();

                sendToKz(CMD_RM_SETTING, mSendBuf, i);

                ui->pushButton_qingout_2->setText("打开");
                show_PromptInformation("排气阀常开：关！");
                ui->pushButton_qingout_2->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
            }
            mQingOut_Flag2 = !mQingOut_Flag2;
        }

    });

    connect(ui->pushButton_WXRead, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            uint32_t i = 0;
            uint32_t tempuint;
            float tempfl;

            mSendBuf[i++] = 0X01;
            mSendBuf[i++] = ui->lineEdit_cycleTIM->text().toUInt();

            tempfl = ui->lineEdit_Exhausttime_Y1->text().toFloat();
            tempuint = tempfl * 10;
            mSendBuf[i++] = (uint8_t)(tempuint);
            mSendBuf[i++] = (uint8_t)(tempuint >> 8);

            // 燃料电池温度上限
            tempfl = ui->lineEdit_WXranliaoTemperMAX->text().toFloat();
            tempuint = tempfl * 10;
            mSendBuf[i++] = (uint8_t)(tempuint);
            mSendBuf[i++] = (uint8_t)(tempuint >> 8);

            // 燃料电池运行目标温度
            tempfl = ui->lineEdit_WXranliaoTargetTemper->text().toFloat();
            tempuint = tempfl * 10;
            mSendBuf[i++] = (uint8_t)(tempuint);
            mSendBuf[i++] = (uint8_t)(tempuint >> 8);

            mSendBuf[i++] = ui->lineEdit_GFJ_PWM->text().toUInt();

            sendToKz(CMD_RM_SETTING, mSendBuf, i);

        }
    });

    connect(ui->pushButton_WXWrite, &QPushButton::clicked, this, &Dialog::WXSetParsing);

    connect(ui->pushButton_WXInit, &QPushButton::clicked, [=](){
        if (mWeixiu_Flag)
        {
            uint32_t i = 0;
            uint32_t tempuint;
            float tempfl;

            mSendBuf[i++] = 0X03;
            mSendBuf[i++] = ui->lineEdit_cycleTIM->text().toUInt();

            tempfl = ui->lineEdit_Exhausttime_Y1->text().toFloat();
            tempuint = tempfl * 10;
            mSendBuf[i++] = (uint8_t)(tempuint);
            mSendBuf[i++] = (uint8_t)(tempuint >> 8);

            // 燃料电池温度上限
            tempfl = ui->lineEdit_WXranliaoTemperMAX->text().toFloat();
            tempuint = tempfl * 10;
            mSendBuf[i++] = (uint8_t)(tempuint);
            mSendBuf[i++] = (uint8_t)(tempuint >> 8);

            // 燃料电池运行目标温度
            tempfl = ui->lineEdit_WXranliaoTargetTemper->text().toFloat();
            tempuint = tempfl * 10;
            mSendBuf[i++] = (uint8_t)(tempuint);
            mSendBuf[i++] = (uint8_t)(tempuint >> 8);

            mSendBuf[i++] = ui->lineEdit_GFJ_PWM->text().toUInt();

            sendToKz(CMD_RM_SETTING, mSendBuf, i);
        }
        else
        {
            show_PromptInformation_Red("请先打开维修模式！");
        }
    });

    connect(ui->pushButton_moni, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            if (mMoNi_Flag)
            {
                uint32_t i = 0;
                mSendBuf[i++] = MODE_IMITATE;
                mSendBuf[i++] = OPEN;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_moni->setText("模拟关");
                show_PromptInformation("模拟：开！！！");
                ui->pushButton_moni->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint32_t i = 0;
                mSendBuf[i++] = MODE_IMITATE;
                mSendBuf[i++] = CLOSE;
                sendToKz(CMD_NMOP_SWITCH, mSendBuf, i);
                ui->pushButton_moni->setText("模拟开");
                show_PromptInformation("模拟：关！！！");
                ui->pushButton_moni->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
            }
            mMoNi_Flag = !mMoNi_Flag;
        }
    });

    connect(ui->pushButton_saveFlash, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            uint8_t buf[10] = {0xAA, 0xBB, 0xF0, 0xA2, 0x0C, 0x01, 0x01, 0x05};
            uint8_t len = sizeof(buf);
            send_Data(buf,len);
        }
    });
}

Dialog::~Dialog()
{
    delete ui;
}

// 获取串口配置
void Dialog::getSerialPortConfig()
{
    mPortName = ui->CBoxSerialPort->currentText();
    mBaudRate = ui->CBoxBaud->currentText();
    mParity   = ui->CBoxParity->currentText();
    mDataBits = ui->CBoxDataBit->currentText();
    mStopBits = ui->CBoxStopBit->currentText();
}

// 设置串口配置
bool Dialog::setSerialPortConfig()
{
    // 设置端口号
    mSerialPort->setPortName(mPortName);

    // 设置波特率
    if ("9600" == mBaudRate)
    {
        mSerialPort->setBaudRate(QSerialPort::Baud9600);
    }
    else if ("19200" == mBaudRate)
    {
        mSerialPort->setBaudRate(QSerialPort::Baud19200);
    }
    else
    {
        mSerialPort->setBaudRate(QSerialPort::Baud115200);
    }

    // 设置校验位
    if ("EVEN" == mParity)
    {
        mSerialPort->setParity(QSerialPort::EvenParity);
    }
    else if ("ODD" == mParity)
    {
        mSerialPort->setParity(QSerialPort::OddParity);
    }
    else
    {
        mSerialPort->setParity(QSerialPort::NoParity);
    }

    // 设置数据位
    if ("5" == mDataBits)
    {
        mSerialPort->setDataBits(QSerialPort::Data5);
    }
    else if ("6" == mDataBits)
    {
        mSerialPort->setDataBits(QSerialPort::Data6);
    }
    else if ("7" == mDataBits)
    {
        mSerialPort->setDataBits(QSerialPort::Data7);
    }
    else
    {
        mSerialPort->setDataBits(QSerialPort::Data8);
    }

    // 设置停止位
    if ("1.5" == mStopBits)
    {
        mSerialPort->setStopBits(QSerialPort::OneAndHalfStop);
    }
    else if ("2" == mStopBits)
    {
        mSerialPort->setStopBits(QSerialPort::TwoStop);
    }
    else
    {
        mSerialPort->setStopBits(QSerialPort::OneStop);
    }

    if (mSerialPort->isOpen()) {
        mSerialPort->close();
        qDebug() << "Closed the port" << mSerialPort->portName();
    } else {
        qDebug() << mSerialPort->portName() << "is closed";
    }

    // 返回串口打开结果 true/false
    bool suc = mSerialPort->open(QIODevice::WriteOnly);
    if (!suc) {
        qDebug() << "Failed to open port" << mSerialPort->portName()
                 << "Error:" << mSerialPort->errorString()
                 << "Error Code:" << mSerialPort->error();
        return 1;
    } else {
        qDebug() << "success to open port" << mSerialPort->portName()
                 << "  BaudRate" << mSerialPort->baudRate()
                 << "  Parity" << mSerialPort->parity()
                 << "  dataBits" << mSerialPort->dataBits()
                 << "  stopBits" << mSerialPort->stopBits();
    }
    return suc;
}

void Dialog::show_PromptInformation(QString str)
{
    mPal.setColor(QPalette::Text, QColor(0, 85, 0));
    // 在LWidPrtInfo中加入item
    ui->LWidPrtInfo->setPalette(mPal);
    ui->LWidPrtInfo->clear();
    ui->LWidPrtInfo->addItem(str);
}

void Dialog::show_PromptInformation_Red(QString str)
{
    mPal.setColor(QPalette::Text, QColor(Qt::red));
    ui->LWidPrtInfo->setPalette(mPal);
    ui->LWidPrtInfo->clear();
    ui->LWidPrtInfo->addItem(str);
}

void Dialog::on_BtnOpen_clicked()
{
    if (true == mIsOpen)
    {
        // 当前串口已经打开，应该执行关闭动作
        mSerialPort->close();
        ui->BtnOpen->setText("打开");
        mIsOpen = false;
        show_PromptInformation("关闭串口成功");
        ui->BtnOpen->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");

        // 打开下拉框
        //        ui->BtnSend->setEnabled(mIsOpen);
        ui->CBoxSerialPort->setEnabled(true);
        ui->CBoxBaud->setEnabled(true);
        ui->CBoxParity->setEnabled(true);
        ui->CBoxDataBit->setEnabled(true);
        ui->CBoxStopBit->setEnabled(true);
    }
    else
    {
        // 没有打开串口，应该执行打开动作
        getSerialPortConfig();
        if (true == setSerialPortConfig())
        {
            mIsOpen = true;
            ui->BtnOpen->setText("关闭");
            show_PromptInformation("打开串口成功");
            ui->BtnOpen->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            // 禁止使用下拉框
            //            ui->BtnSend->setEnabled(mIsOpen);
            ui->CBoxSerialPort->setEnabled(false);
            ui->CBoxBaud->setEnabled(false);
            ui->CBoxParity->setEnabled(false);
            ui->CBoxDataBit->setEnabled(false);
            ui->CBoxStopBit->setEnabled(false);
        }

    }
}

unsigned char Dialog::checkcrc8(uint8_t * buf ,uint32_t len)
{
    uint32_t sum = 0;
    uint32_t i;
    for(i=0;i<len;i++)
    {
        sum+=buf[i];
    }
    return (uint8_t)sum;
}

unsigned char Dialog::checkcrcBA(QByteArray buf ,int len)
{
    uint8_t sum = 0;
    uint8_t i;
    for(i=0;i<len;i++)
    {
        sum+=buf[i];
    }
    sum = sum % 256;
    return sum;

}

float Dialog::uint32ToFloat(uint32_t intValue)
{
    myUnion.uint = intValue;
    float floatValue = myUnion.floatVal;

    return floatValue;
}

// 解析数据
void Dialog::ParsingData()
{
    // if(mRec_Buf[0]!=0xAA || mRec_Buf[1]!=0xBB || mRec_Buf[3]!=0x2A)  //校验协议
    // {
    //     mRec_cnt = 0;
    //     return;
    // }

    if(mRec_Buf[OFFSET_CRC] != checkcrc8(mRec_Buf + OFFSET_CMD, mRec_Buf[OFFSET_LEN] + SZ_LEN + SZ_CMD))
    {
        mRec_cnt = 0;
        return;
    }

    if (mRec_Buf[OFFSET_CMD] == CMD_PERODIC_QUERY)
    {
        memcpy(UN_Cyc_InquireData.G_Cyc_InquireData_8, mRec_Buf + SZ_OVERHEAD, mRec_Buf[OFFSET_LEN]);
        // Byte6   报警信息
        //if ((mRec_Buf[i] &0x01) == 1)   // bit0 过温
        //{
        //    ui->label_temp_flag->setText("温度高");
        //    ui->label_temp_flag->setStyleSheet("background-color: rgb(225, 0, 0);");
        //    mTemp_Flag = "1";
        //}
        //else
        //{
        //    ui->label_temp_flag->setText("正常");
        //    ui->label_temp_flag->setStyleSheet("background-color: rgb(85, 170, 127);");
        //    mTemp_Flag = "0";
        //}

        //if ((mRec_Buf[i] &0x02) == 2)   // bit1 氢量低
        //{
        //    ui->label_qing_flag->setText("氢量低");
        //    ui->label_qing_flag->setStyleSheet("background-color: rgb(225, 0, 0);");
        //    mQing_Flag = "1";
        //}
        //else
        //{
        //    ui->label_qing_flag->setText("正常");
        //    ui->label_qing_flag->setStyleSheet("background-color: rgb(85, 170, 127);");
        //    mQing_Flag = "0";
        //}

        //if ((mRec_Buf[i] &0x04) == 4)   // bit2 欠压
        //{
        //    ui->label_V_flag->setText("电压低");
        //    ui->label_V_flag->setStyleSheet("background-color: rgb(225, 0, 0);");
        //    mV_Flag = "1";
        //}
        //else
        //{
        //    ui->label_V_flag->setText("正常");
        //    ui->label_V_flag->setStyleSheet("background-color: rgb(85, 170, 127);");
        //    mV_Flag = "0";
        //}

        //i++;
        // Byte7    锂电池SOC
        ui->label_lidiansoc->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.Li_SOC);

        // Byte[10-13]  锂电池电压
        ui->label_lidianV->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.Li_Voltage, 'f', 1));

        // Byte[14-17]  锂电池电流
        ui->label_lidianA->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.Li_Current, 'f', 1));

        // 功率
        ui->label_lidianW->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.Li_Power, 'f', 1));

        // Byte[18-21]  锂电池温度
        ui->label_lidianTemp->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.Li_Temper, 'f', 1));

        // Byte[22-25] 锂电池运行时间
        ui->label_lidianruntime->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.Li_RunTime, 'f', 2));

        // Byte[29-26] 燃料电压
        ui->label_ranliaoV->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_Voltage, 'f', 1));

        // Byte[30-33] 燃料电流
        ui->label_ranliaoA->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_Current, 'f', 1));

        // 功率
        ui->label_ranliaoW->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_Power, 'f', 1));

        // Byte[34-37] 燃料温度
        ui->label_ranliaoTemp->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_Temper, 'f', 1));

        if (mWeixiu_Flag)
        {
            ui->label_ranliaoTemp_2->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_Temper, 'f', 1));
        }

        // Byte[38-41] 燃料运行时间
        ui->label_ranliaoruntime->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_AllRunTime, 'f', 2));

        // Byte[42] 风扇占空比
        ui->label_fspwm->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_FanDuty);

        // Byte[44-45] 风扇转速
        ui->label_fszhuansu->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_FanSpeed);

        if (mWeixiu_Flag)
        {
            ui->label_fszhuansu_2->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_FanSpeed);
        }

        // Byte[46] 鼓风机占空比
        ui->label_gfjpwm->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_BlowerDuty);

        // Byte[48-49] 鼓风机转速
        ui->label_gfjzhuansu->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_BlowerSpeed);

        if (mWeixiu_Flag)
        {
            ui->label_gfjzhuansu_2->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_BlowerSpeed);
        }

        // Byte[50-51] 输出电量
        ui->label_qingoutdl->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HJ_Outpower);

        //// Byte[52-53] 氢罐剩余容量
        //tempint = ((mRec_Buf[i] | mRec_Buf[i+1] << 8));i += 2;
        //ui->label_qingsyrl->setNum((uint16_t)tempint);

        //// Byte[54-55] 氢罐剩余里程
        //tempint = ((mRec_Buf[i] | mRec_Buf[i+1] << 8));i += 2;
        //ui->label_qingsylc->setNum((uint16_t)tempint);

        //// Byte[56-57] 字节对齐
        //i += 2;

        //// Byte[58-61] 总剩余里程
        //tempint = ((mRec_Buf[i] | mRec_Buf[i+1] << 8 | mRec_Buf[i+2] << 16 | mRec_Buf[i+3] << 24));i += 4;
        //ui->label_zsylc->setNum(tempint);

        //// Byte[62-65] 负载端功率
        //tempint = ((mRec_Buf[i] | mRec_Buf[i+1] << 8 | mRec_Buf[i+2] << 16 | mRec_Buf[i+3] << 24));i += 4;
        //if (tempint <= 0)
        //{
        //    tempint = 0;
        //}
        //ui->label_fzdgl->setNum(tempint);

        // Byte[66-69] 	氢气总容量
        ui->label_qzrl->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_AllCap);

    }

    else if (mRec_Buf[OFFSET_CMD] == CMD_RM_SETTING)
    {
        memcpy(UN_Normal_SetDate.G_Normal_SetDate_8, mRec_Buf + SZ_OVERHEAD, mRec_Buf[OFFSET_LEN]);

        //锂电池参数设置
        // 锂电池可用Soc下限电压
        ui->lineEdit_lidiankeyongSocXiaX->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_SOCMin));

        // 锂电池可用Soc上限电压
        ui->lineEdit_lidiankeyongSocShangX->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_SOCMax));

        // 电堆充电触发Soc
        ui->lineEdit_startchongdianSoc->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_ChargeSoc));

        // 电堆停止充电截止Soc
        ui->lineEdit_stopchongdianSoc->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_StopSoc));

        // 锂电池安全下限电压
        ui->lineEdit_anqunVXiaX->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_Volmin, 'f', 1));

        // Byte[9] 充满换罐次数
        ui->lineEdit_LI_FullCharg_Cyc->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_FullCharg_Cyc));

        //氢罐参数设置
        // Byte[10-11] 氢罐初始容量
        ui->lineEdit_qingzongrongliang->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_IntCap));

        // Byte[12-13] 氢罐最低电量
        ui->lineEdit_qingzuidizhi->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_MinCap));

        // Byte[14-15] 清瓶模式时触发氢罐容量
        ui->lineEdit_qingqingpingrl->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_ClearCap_H));

        // Byte[16-17] 清瓶模式时触发锂电池容量
        ui->lineEdit_qingqingpingLDSOC->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_ClearCap_B));

        //电堆参数设置
        //// Byte[18] 冷却风扇常开pwm
        //tempint = mRec_Buf[i++] ;
        //ui->lineEdit_fengshanChangkaipwm->setText(QString::number(tempint));

        // Byte[19] 鼓风机最小pwm
        ui->lineEdit_gufengjipwmMin->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_PWM_min));

        //// Byte[20] 鼓风机常开pwm
        //tempint = mRec_Buf[i++] ;
        //ui->lineEdit_gufengjiChangkaipwm->setText(QString::number(tempint));

        // Byte[29] 鼓风机开机前运行时间
        ui->lineEdit_gfjKaiJiRuntime->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.BL_RunStartTim));

        // Byte[21] 电堆超温持续时间
        ui->lineEdit_dianduiOverTempertime->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.BL_OvertepTim));

        // Byte[22] 电堆吹扫持续时间
        ui->lineEdit_dianduiChiXuChuiFengtime->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.BL_HoldTim));

        // Byte[25-28] 燃料电池电压下限
        ui->lineEdit_ranliaoDCVMin->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_Vomin, 'f', 1));

        // Byte[29-32] 电堆电流下限
        ui->lineEdit_dianDuiDinliuMin->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_Iomin, 'f', 1));

        // Byte[33-36] 燃料电池温度上限
        ui->lineEdit_ranliaoTemperMAX->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_Tepmax, 'f', 1));

        // Byte[37-40] 燃料电池运行目标温度
        ui->lineEdit_ranliaoTargetTemper->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_TepTarge, 'f', 1));

        // Byte[41-44] 氢电池系数A
        ui->lineEdit_gufengjipwmA->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_PWM_A, 'f', 1));

        // Byte[45-48] 氢电池系数B
        ui->lineEdit_gufengjipwmB->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_PWM_B, 'f', 1));

        //排气参数
        // Byte[53] 间隔时间
        ui->lineEdit_paiqiJiangetime->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_Interval));

        // Byte[57-60] 排气时间-1
        ui->lineEdit_paiqitime_1->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_Time1, 'f', 1));

        // Byte[61-64] 排气时间-2
        ui->lineEdit_paiqitime_2->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_Time2, 'f', 1));

        // Byte[65] 电堆开机排氢次数
        ui->lineEdit_ddKaiJiPaiqingcnt->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_StartTimes));

        // Byte[67-68] 电堆堵水时功率预警值
        ui->lineEdit_dddushuiPower->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_WaterP));

        // Byte[69] 电堆堵水时排气阀循环数
        ui->lineEdit_ddduishuipaiqicnt->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_CycleTimes));

        //鼓风机循环参数
        // Byte[73-74] 鼓风机持续间隔时间
        ui->lineEdit_gfjjiangetime->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_Interval));

        // Byte[75] 鼓风机开持续时间
        ui->lineEdit_gfjchixutimeOpen->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_OpenTim));

        // Byte[76] 鼓风机关持续时间
        ui->lineEdit_gfjchixutimeClose->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_CloseTim));

        // Byte[77] 鼓风机运行次数
        ui->lineEdit_gfjyunxingcishu->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_SSTimes));

        // Byte[77] 第一次亏空时间
        ui->lineEdit_BLCyc_IntervalFirst->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_IntervalFirst));

        show_PromptInformation("读取参数成功！");
    }
    else if (mRec_Buf[4] == 0x03)
    {
        show_PromptInformation("设置参数成功！");
    }
    else if (mRec_Buf[4] == 0x04)
    {
        show_PromptInformation("初始化参数成功！");
    }
    else if (mRec_Buf[OFFSET_CMD] == CMD_RM_SETTING)
    {
        memcpy(UN_Repair_InquireData.G_Repair_InquireData_8, mRec_Buf + SZ_OVERHEAD, mRec_Buf[OFFSET_LEN]);

        ui->lineEdit_WXranliaoTemperMAX->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.HB_Tepmax, 'f', 1));

        ui->lineEdit_WXranliaoTargetTemper->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.HB_TepTarge, 'f', 1));

        ui->lineEdit_GFJ_PWM->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.HB_BlowerDuty));

        ui->lineEdit_cycleTIM->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.EH_Interval));

        ui->lineEdit_Exhausttime_Y1->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.EH_Time1, 'f', 1));

        show_PromptInformation("维修模式读取参数成功！");
    }
    else if (mRec_Buf[4] == 0x08)
    {
        show_PromptInformation("维修模式设置参数成功！");
    }
    else if (mRec_Buf[4] == 0x0A)
    {
        show_PromptInformation("维修模式初始化参数成功！");
    }
    else if (mRec_Buf[4] == 0x0C)
    {
        show_PromptInformation("写入设置数据成功！");
    }
    //else if (mRec_Buf[4] == 0x0D)
    //{
    //    if (mRec_Buf[6] == 0x01) {
    //        tempint = ((mRec_Buf[i + 1] << 8) | mRec_Buf[i + 2]);
    //        double decimalValue = static_cast<double>(tempint) / 10.0;
    //        ui->tempLineEdit->setText(QString::number(decimalValue, 'f', 1));
    //        show_PromptInformation("查询矫正温度成功！");
    //    } else if (mRec_Buf[6] == 0x02) {
    //        tempint = ((mRec_Buf[i + 1] << 8) | mRec_Buf[i + 2]);
    //        double decimalValue = static_cast<double>(tempint) / 10.0;
    //        ui->tempLineEdit->setText(QString::number(decimalValue, 'f', 1));
    //        show_PromptInformation("设置矫正温度成功！");
    //    }
    //}

    mRec_cnt = 0;
}

void Dialog::sendToKz(uint8_t cmd,uint8_t *buf,uint32_t len)
{
    uint32_t i = 0;

    if(len!=0)
    {
        memcpy(mSendpcbuf + SZ_OVERHEAD, buf, len);
    }
    mSendpcbuf[i++] = HEADER_FIRST_BYTE;
    mSendpcbuf[i++] = HEADER_SECOND_BYTE;
    mSendpcbuf[i++] = SWJADDR;//
    mSendpcbuf[i++] = XWJADDR;//cmd
    mSendpcbuf[i++] = checkcrc8(mSendpcbuf + OFFSET_CMD, len + SZ_CMD + SZ_LEN);
    mSendpcbuf[i++] = cmd;//cmd
    mSendpcbuf[i++] = len; //长度
    send_Data(mSendpcbuf, SZ_OVERHEAD + len);
}

void Dialog::allInit()
{
    mIsOpen = false;
    mHexShowFlag = false;
    mRec_Flag = false;
    mWeixiu_Flag = false;
    mGuFen_Flag = true;
    mFenSan_Flag = true;
    mQingIn_Flag = true;
    mQingOut_Flag = true;
    mQingOut_Flag2 = true;
    mQingIn_Flag2 = true;
    mMoNi_Flag = true;
    mWrite_Flag = false;
    mSave_Flag = false;

    ui->BtnOpen->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_weixiu->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_read->setStyleSheet("background-color: rgb(85, 170, 255); color: rgb(255, 255, 255);");
    ui->pushButton_write->setStyleSheet("background-color: rgb(85, 170, 255); color: rgb(255, 255, 255);");
    ui->pushButton_init->setStyleSheet("background-color: rgb(85, 170, 255); color: rgb(255, 255, 255);");
    ui->pushButton_gfjck->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_fsck->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_qingin->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_qingout->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_qingin_2->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_qingout_2->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_WXRead->setStyleSheet("background-color: rgb(85, 170, 255); color: rgb(255, 255, 255)");
    ui->pushButton_WXWrite->setStyleSheet("background-color: rgb(85, 170, 255); color: rgb(255, 255, 255)");
    ui->pushButton_WXInit->setStyleSheet("background-color: rgb(85, 170, 255); color: rgb(255, 255, 255)");
    ui->pushButton_moni->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_saveStart->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
    ui->pushButton_setPathName->setStyleSheet("background-color: rgb(85, 170, 255); color: rgb(255, 255, 255)");
    ui->pushButton_setSavePath->setStyleSheet("background-color: rgb(85, 170, 255); color: rgb(255, 255, 255)");
    ui->pushButton_saveFlash->setStyleSheet("background-color: rgb(85, 170, 255); color: rgb(255, 255, 255)");

}

void Dialog::SetParsing()
{
    if (mIsOpen)
    {
        uint32_t i = 0;
        uint32_t tempuint;
        float tempfl;

        /* 电堆参数设置 */
        // 氢电池输出电压下限
        tempfl = ui->lineEdit_ranliaoDCVMin->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 氢电池温度上限
        tempfl = ui->lineEdit_ranliaoTemperMAX->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 氢电池电流下限
        tempfl = ui->lineEdit_dianDuiDinliuMin->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 氢电池目标温度
        tempfl = ui->lineEdit_ranliaoTargetTemper->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 氢电池系数A
        tempfl = ui->lineEdit_gufengjipwmA->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 氢电池系数B
        tempfl = ui->lineEdit_gufengjipwmB->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 电堆超温持续时间
        mSendBuf[i++] = ui->lineEdit_dianduiOverTempertime->text().toUInt();

        // 电堆扫风持续时间
        mSendBuf[i++] = ui->lineEdit_dianduiChiXuChuiFengtime->text().toUInt();

        // 鼓风机开机前运行时间
        mSendBuf[i++] = ui->lineEdit_gfjKaiJiRuntime->text().toUInt();

        // 鼓风机最小占空比
        mSendBuf[i++] = ui->lineEdit_gufengjipwmMin->text().toUInt();

        /* 氢罐参数设置 */
        // 氢罐初始容量
        tempuint = ui->lineEdit_qingzongrongliang->text().toUInt();
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 氢罐最低电量
        tempuint = ui->lineEdit_qingzuidizhi->text().toUInt();
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 氢罐清罐氢罐容量
        tempuint = ui->lineEdit_qingqingpingrl->text().toUInt();
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 氢罐清罐电池容量
        tempuint = ui->lineEdit_qingqingpingLDSOC->text().toUInt();
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        /* 鼓风机循环参数 */
        // 第一次亏空时间
        mSendBuf[i++] = ui->lineEdit_BLCyc_IntervalFirst->text().toUInt();

        // 循环间隔时间
        tempuint = ui->lineEdit_gfjjiangetime->text().toUInt();
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 开持续时间
        mSendBuf[i++] = ui->lineEdit_gfjchixutimeOpen->text().toUInt();

        // 关持续时间
        mSendBuf[i++] = ui->lineEdit_gfjchixutimeClose->text().toUInt();

        // 启停次数
        mSendBuf[i++] = ui->lineEdit_gfjyunxingcishu->text().toUInt();

        /* 排气参数 */
        // 排气间隔时间
        mSendBuf[i++] = ui->lineEdit_paiqiJiangetime->text().toUInt();

        // 排气时间-1
        tempfl = ui->lineEdit_paiqitime_1->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 排气时间-2
        tempfl = ui->lineEdit_paiqitime_2->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 堵水功率
        tempuint = ui->lineEdit_dddushuiPower->text().toUInt();
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 开机排气次数
        mSendBuf[i++] = ui->lineEdit_ddKaiJiPaiqingcnt->text().toUInt();

        // 堵水排气次数
        mSendBuf[i++] = ui->lineEdit_ddduishuipaiqicnt->text().toUInt();

        /* 锂电池参数 */
        // 锂电池soc上限电压
        mSendBuf[i++] = ui->lineEdit_lidiankeyongSocShangX->text().toUInt();

        // 锂电池soc下限电压
        mSendBuf[i++] = ui->lineEdit_lidiankeyongSocXiaX->text().toUInt();

        // 充电触发Soc
        mSendBuf[i++] = ui->lineEdit_startchongdianSoc->text().toUInt();

        // 充电截止Soc
        mSendBuf[i++] = ui->lineEdit_stopchongdianSoc->text().toUInt();

        // 锂电池电压下限
        tempfl = ui->lineEdit_anqunVXiaX->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 满充换罐次数
        mSendBuf[i++] = ui->lineEdit_LI_FullCharg_Cyc->text().toUInt();

        sendToKz(CMD_NORMAL_PARAMS, mSendBuf, i);
    }

}

void Dialog::WXSetParsing()
{
    if (mWeixiu_Flag)
    {
        uint32_t i = 0;
        uint32_t tempuint;
        float tempfl;
        mSendBuf[i++] = 0X02;
        mSendBuf[i++] = ui->lineEdit_cycleTIM->text().toUInt();

        tempfl = ui->lineEdit_Exhausttime_Y1->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 燃料电池温度上限
        tempfl = ui->lineEdit_WXranliaoTemperMAX->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        // 燃料电池运行目标温度
        tempfl = ui->lineEdit_WXranliaoTargetTemper->text().toFloat();
        tempuint = tempfl * 10;
        mSendBuf[i++] = (uint8_t)(tempuint);
        mSendBuf[i++] = (uint8_t)(tempuint >> 8);

        mSendBuf[i++] = ui->lineEdit_GFJ_PWM->text().toUInt();

        sendToKz(CMD_RM_SETTING, mSendBuf, i);
    }
}

void Dialog::SaveData()
{
    QVariantMap data;
    QDateTime currentTime = QDateTime::currentDateTime();
    QString current_date = currentTime.toString("hh:mm:ss:zzz");
    QString path_name = currentTime.toString(u8"运行参数_yyyy_MM_dd");
    data.insert(u8"时间", current_date);
    data.insert(u8"温度报警", mTemp_Flag);
    data.insert(u8"氢罐报警", mQing_Flag);
    data.insert(u8"电压低报警", mV_Flag);
    data.insert(u8"锂电soc", ui->label_lidiansoc->text());
    data.insert(u8"锂电电压", ui->label_lidianV->text());
    data.insert(u8"锂电电流", ui->label_lidianA->text());
    data.insert(u8"锂电功率", ui->label_lidianW->text());
    data.insert(u8"锂电温度", ui->label_lidianTemp->text());
    data.insert(u8"锂电运行时间", ui->label_lidianruntime->text());
    data.insert(u8"燃料电压", ui->label_ranliaoV->text());
    data.insert(u8"燃料电流", ui->label_ranliaoA->text());
    data.insert(u8"燃料功率", ui->label_ranliaoW->text());
    data.insert(u8"燃料温度", ui->label_ranliaoTemp->text());
    data.insert(u8"燃料运行时间", ui->label_ranliaoruntime->text());
    data.insert(u8"风扇PWM", ui->label_fspwm->text());
    data.insert(u8"风扇转速", ui->label_fszhuansu->text());
    data.insert(u8"鼓风机PWM", ui->label_gfjpwm->text());
    data.insert(u8"鼓风机转速", ui->label_gfjzhuansu->text());
    data.insert(u8"氢罐输出电量", ui->label_qingoutdl->text());
    data.insert(u8"氢罐剩余电量", ui->label_qingsyrl->text());
    data.insert(u8"氢罐剩余里程", ui->label_qingsylc->text());
    data.insert(u8"氢罐总剩余里程", ui->label_zsylc->text());
    data.insert(u8"负载端功率", ui->label_fzdgl->text());

    if (!(ui->lineEdit_pathName->text().isEmpty()))
    {
        path_name = mPathName;
    }

    path_name += ".csv";
    if (mSavePath == nullptr)
    {
        QString Path = QDir::current().absolutePath() + "/rec";
        SaveData::Singleton()->SaveDataToLocal(Path, path_name, data);
    }
    else
    {
        SaveData::Singleton()->SaveDataToLocal(mSavePath, path_name, data);
    }
}

void Dialog::RecData()
{
    qDebug() << "RecData";
    if (true == mIsOpen)
    {

        uint8_t rOneByte = 0;
        QByteArray recvData = mSerialPort->readAll();

        char *pdata = recvData.data();
        for (uint16_t i = 0; i < recvData.size(); ++i){
            qDebug("<0x%x>",  *(uint8_t *)(pdata + i));
        }
        mTimer2->stop();
        if (recvData.isEmpty())
            return;

        int i = 0;
        while (mRec_cnt < 100) {
            rOneByte = recvData.at(i++);
            mRec_Buf[mRec_cnt++] = rOneByte;

            if (mRec_Buf[0] == HEADER_FIRST_BYTE)
            {
                if (mRec_cnt >= 5)
                {
                    if (mRec_Buf[1] == HEADER_SECOND_BYTE  && mRec_Buf[OFFSET_SND_ADDR] == XWJADDR)
                    {
                        if (mRec_Buf[OFFSET_SND_ADDR] == SWJADDR)
                        {
                            if (mRec_Buf[OFFSET_LEN] + SZ_OVERHEAD ==  mRec_cnt)
                            {
                                ParsingData();
                                return;
                            }
                        }
                        else
                        {
                            mRec_cnt = 0;
                        }
                    }
                    else
                    {
                        mRec_cnt = 0;
                    }
                }
            }
            else
            {
                mRec_cnt = 0;
            }
        }

        if (mRec_cnt >= 100)
        {
            mRec_cnt = 0;
        }
    }

}


void Dialog::UpdatePort()
{

    // 智能识别当前系统有效端口号
    QStringList newPortStringList;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo & info : infos)
    {
        newPortStringList += info.portName();
    }
    // 更新串口号
    if (newPortStringList.size() != oldPortStringList.size())
    {
        oldPortStringList = newPortStringList;
        ui->CBoxSerialPort->clear();
        ui->CBoxSerialPort->addItems(oldPortStringList);
    }
}

void Dialog::send_Data(uint8_t *buf, uint8_t len)
{
    for (uint i = 0; i < len; ++i){
        qDebug("<0x%x>", buf[i]);
    }
    qDebug("1111111111111111111111");
    //mSerialPort->write(reinterpret_cast<char *>(buf), len);
    qint64 bytesWritten = mSerialPort->write(reinterpret_cast<char *>(buf), len);
    if (bytesWritten == len) {
        // 成功发送所有数据
        qDebug("send datas successful");
    } else {
        // 发送不完全或发生错误
        qDebug("send datas defeat");
    }
}



//void Dialog::on_selectPushButton_clicked()
//{
//	uint32_t i = 0;
//	mSendBuf[i++] = 0x01;
//	mSendBuf[i++] = 0x00;
//	mSendBuf[i++] = 0x00;
//	sendToKz(0x0D, mSendBuf, i);
//}
//
//void Dialog::on_settingPushButton_clicked()
//{
//	QString input = ui->tempLineEdit->text();
//	double value = input.toDouble();
//	int decimalValue = static_cast<int>(value * 10);
//	uint32_t i = 0;
//	mSendBuf[i++] = 0x02;
//	mSendBuf[i++] = (decimalValue >> 8) & 0xFF;
//	mSendBuf[i++] = decimalValue & 0xFF;
//	sendToKz(0x0D, mSendBuf, i);
//}

void Dialog::periodicQuery()
{
    uint8_t buf[1] = { 0 };
    buf[0] = 0XAB;
    sendToKz(CMD_PERODIC_QUERY, buf, 1);
    periodicQueryTimer->start(PERIODIC_QUERY_TIME);
}
