#include "dialog.h"
#include "ui_dialog.h"
#include "savedata.h"
#include "param.h"

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
        SetParsing();
    });

    connect(ui->pushButton_read, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            uint8_t buf[1] = { 0 };
            buf[0] = 0X01;
            sendToKz(CMD_NORMALPARAM_READ, buf, 1);
        }
    });

    connect(ui->pushButton_init, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            uint8_t buf[1] = { 0 };
            buf[0] = 0X02;
            sendToKz(CMD_NORMALPARAM_READ, buf, 1);
        }
    });

    connect(ui->pushButton_weixiu, &QPushButton::clicked, [=](){
        if (mWeixiu_Flag)
        {
            uint8_t buf[2] = { 0 };
            buf[0] = MODE_MAINTAIN;
            buf[1] = OPEN;
            sendToKz(CMD_MODE_SWITCH, buf, 2);
            ui->pushButton_weixiu->setText("维修模式开");
            ui->pushButton_weixiu->setStyleSheet("background-color: rgb(85, 170, 127); color: rgb(255, 255, 255)");
        }
        else
        {
            uint8_t buf[2] = { 0 };
            buf[0] = MODE_MAINTAIN;
            buf[1] = CLOSE;
            sendToKz(CMD_MODE_SWITCH, buf, 2);
            ui->pushButton_weixiu->setText("维修模式关");
            ui->pushButton_weixiu->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
        }
        mWeixiu_Flag = !mWeixiu_Flag;
    });

    connect(ui->pushButton_gfjck, &QPushButton::clicked, [=](){
        if (mIsOpen)
        {
            if (mGuFen_Flag)
            {
                uint8_t buf[2] = { 0 };
                buf[0] = DEVICE_BLOWER;
                buf[1] = OPEN;
                sendToKz(CMD_NMOP_SWITCH, buf, 2);
                ui->pushButton_gfjck->setText("关闭");
                ui->pushButton_gfjck->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint8_t buf[2] = { 0 };
                buf[0] = DEVICE_BLOWER;
                buf[1] = CLOSE;
                sendToKz(CMD_NMOP_SWITCH, buf, 2);
                ui->pushButton_gfjck->setText("打开");
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
                uint8_t buf[2] = { 0 };
                buf[0] = DEVICE_FAN;
                buf[1] = OPEN;
                sendToKz(CMD_NMOP_SWITCH, buf, 2);
                ui->pushButton_fsck->setText("关闭");
                ui->pushButton_fsck->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint8_t buf[2] = { 0 };
                buf[0] = DEVICE_FAN;
                buf[1] = CLOSE;
                sendToKz(CMD_NMOP_SWITCH, buf, 2);
                ui->pushButton_fsck->setText("打开");
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
                uint8_t buf[2] = { 0 };
                buf[0] = DEVICE_VALVEIN;
                buf[1] = OPEN;
                sendToKz(CMD_NMOP_SWITCH, buf, 2);
                ui->pushButton_qingin->setText("关闭");
                ui->pushButton_qingin->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint8_t buf[2] = { 0 };
                buf[0] = DEVICE_VALVEIN;
                buf[1] = CLOSE;
                sendToKz(CMD_NMOP_SWITCH, buf, 2);
                ui->pushButton_qingin->setText("打开");
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
                uint8_t buf[2] = { 0 };
                buf[0] = DEVICE_VALVEOUT;
                buf[1] = OPEN;
                sendToKz(CMD_NMOP_SWITCH, buf, 2);
                ui->pushButton_qingout->setText("关闭");
                ui->pushButton_qingout->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint8_t buf[2] = { 0 };
                buf[0] = DEVICE_VALVEOUT;
                buf[1] = CLOSE;
                sendToKz(CMD_NMOP_SWITCH, buf, 2);
                ui->pushButton_qingout->setText("打开");
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
        if (mWeixiu_Flag)
        {
            uint8_t buf[14] = { 0 };
            buf[0] = 0X01;
            sendToKz(CMD_RM_SETTING, buf, 14);
        } else {
            show_PromptInformation_Red("请先打开维修模式！");
        }
    });

    connect(ui->pushButton_WXWrite, &QPushButton::clicked, this, &Dialog::WXSetParsing);

    connect(ui->pushButton_WXInit, &QPushButton::clicked, [=](){
        if (mWeixiu_Flag)
        {
            uint8_t buf[14] = { 0 };
            buf[0] = 0X03;
            sendToKz(CMD_RM_SETTING, buf, 14);
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
                uint8_t buf[2] = { 0 };
                buf[0] = MODE_IMITATE;
                buf[1] = OPEN;
                sendToKz(CMD_MODE_SWITCH, buf, 2);
                ui->pushButton_moni->setText("模拟关");
                ui->pushButton_moni->setStyleSheet("background-color: rgb(0, 85, 0); color: rgb(255, 255, 255)");
            }
            else
            {
                uint8_t buf[2] = { 0 };
                buf[0] = MODE_IMITATE;
                buf[1] = CLOSE;
                sendToKz(CMD_MODE_SWITCH, buf, 2);
                ui->pushButton_moni->setText("模拟开");
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
    bool suc = mSerialPort->open(QIODevice::ReadWrite);
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
        // ui->BtnSend->setEnabled(mIsOpen);
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
            // ui->BtnSend->setEnabled(mIsOpen);
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
    qDebug("checkcrc8");
    uint32_t sum = 0;
    uint32_t i;
    for(i=0;i<len;i++)
    {
        sum+=buf[i];
    }
    qDebug("2222222 The sum : <0x%x>", (uint8_t)sum);
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
    if(mRec_Buf[OFFSET_CRC] != checkcrc8(mRec_Buf + OFFSET_CMD, mRec_Buf[OFFSET_LEN] + SZ_LEN + SZ_CMD))
    {
        qDebug() << "mRec_Buf[OFFSET_CRC] != checkcrc8(mRec_Buf + OFFSET_CMD, mRec_Buf[OFFSET_LEN] + SZ_LEN + SZ_CMD)";
        mRec_cnt = 0;
        return;
    }

    if (mRec_Buf[OFFSET_CMD] == CMD_PERODIC_QUERY)
    {
        qDebug() << "mRec_Buf[OFFSET_CRC] == checkcrc8(mRec_Buf + OFFSET_CMD, mRec_Buf[OFFSET_LEN] + SZ_LEN + SZ_CMD)";
        memcpy(UN_Cyc_InquireData.G_Cyc_InquireData_8, mRec_Buf + SZ_OVERHEAD, mRec_Buf[OFFSET_LEN]);

        // 锂电池电压
        ui->label_lidianV->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.Li_Voltage, 'f', 1));

        // 锂电池电流
        ui->label_lidianA->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.Li_Current, 'f', 1));

        // 锂电池功率
        ui->label_lidianW->setText(QString::number((UN_Cyc_InquireData.G_Cyc_InquireData.Li_Voltage * UN_Cyc_InquireData.G_Cyc_InquireData.Li_Current), 'f', 1));

        //负载端功率
        ui->label_fzdgl->setText(QString::number((UN_Cyc_InquireData.G_Cyc_InquireData.HB_Voltage * UN_Cyc_InquireData.G_Cyc_InquireData.HB_Current) - (UN_Cyc_InquireData.G_Cyc_InquireData.Li_Voltage * UN_Cyc_InquireData.G_Cyc_InquireData.Li_Current) ,'f', 1));

        // 锂电池温度
        ui->label_lidianTemp->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.Li_Temper, 'f', 1));

        // 锂电池运行时间
        ui->label_lidianruntime->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.Li_RunTime, 'f', 2));

        // 锂电池SOC
        ui->label_lidiansoc->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.Li_SOC);

        // 燃料电压
        ui->label_ranliaoV->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_Voltage, 'f', 1));

        // 燃料电流
        ui->label_ranliaoA->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_Current, 'f', 1));

        // 燃料电池功率
        ui->label_ranliaoW->setText(QString::number((UN_Cyc_InquireData.G_Cyc_InquireData.HB_Voltage * UN_Cyc_InquireData.G_Cyc_InquireData.HB_Current), 'f', 1));

        // 燃料温度
        ui->label_ranliaoTemp->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_Temper, 'f', 1));

        if (mWeixiu_Flag)
        {
            ui->label_ranliaoTemp_2->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_Temper, 'f', 1));
        }

        // 氢气总容量
        ui->label_qzrl->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_AllCap);

        // 风扇占空比
        ui->label_fspwm->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_FanDuty);

        // 风扇转速
        ui->label_fszhuansu->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_FanSpeed);

        if (mWeixiu_Flag)
        {
            ui->label_fszhuansu_2->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_FanSpeed);
        }

        // 鼓风机占空比
        ui->label_gfjpwm->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_BlowerDuty);

        // 鼓风机转速
        ui->label_gfjzhuansu->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_BlowerSpeed);

        if (mWeixiu_Flag)
        {
            ui->label_gfjzhuansu_2->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_BlowerSpeed);
        }

        // 燃料运行时间
        ui->label_ranliaoruntime->setText(QString::number(UN_Cyc_InquireData.G_Cyc_InquireData.HB_AllRunTime, 'f', 2));

        // 更换氢罐后输出电量
        ui->label_qingoutdl->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HJ_Outpower);

        // 更换氢罐后剩余电量
        ui->label_qingsyrl->setNum(UN_Cyc_InquireData.G_Cyc_InquireData.HB_AllCap - UN_Cyc_InquireData.G_Cyc_InquireData.HJ_Outpower);

        // 更换氢罐后剩余运行里程
        ui->label_qingsylc->setText(QString::number((UN_Cyc_InquireData.G_Cyc_InquireData.HB_AllCap - UN_Cyc_InquireData.G_Cyc_InquireData.HJ_Outpower) * 0.567));

        // 总剩余里程
        float dianchisylc = (UN_Cyc_InquireData.G_Cyc_InquireData.Li_SOC - 15) * 48 / 100;
        ui->label_zsylc->setText(QString::number((UN_Cyc_InquireData.G_Cyc_InquireData.HB_AllCap - UN_Cyc_InquireData.G_Cyc_InquireData.HJ_Outpower) * 0.567 + dianchisylc));

        // 故障信息
        if((UN_Cyc_InquireData.G_Cyc_InquireData.Erro & 0x01) == 1){
            ui->label_qqgrl->setText("容量不足");
            ui->label_qqgrl->setStyleSheet("background-color: rgb(225, 0, 0);");
        } else {
            ui->label_qqgrl->setText("正常");
            ui->label_qqgrl->setStyleSheet("background-color: rgb(85, 170, 127);");
        }
        if((UN_Cyc_InquireData.G_Cyc_InquireData.Erro & 0x02) == 2){
            ui->label_qqggy->setText("过压");
            ui->label_qqggy->setStyleSheet("background-color: rgb(225, 0, 0);");
        } else {
            ui->label_qqggy->setText("正常");
            ui->label_qqggy->setStyleSheet("background-color: rgb(85, 170, 127);");
        }
        if((UN_Cyc_InquireData.G_Cyc_InquireData.Erro & 0x04) == 4){
            ui->label_qqgxl->setText("泄露");
            ui->label_qqgxl->setStyleSheet("background-color: rgb(225, 0, 0);");
        } else {
            ui->label_qqgxl->setText("正常");
            ui->label_qqgxl->setStyleSheet("background-color: rgb(85, 170, 127);");
        }
        if((UN_Cyc_InquireData.G_Cyc_InquireData.Erro & 0x08) == 8){
            ui->label_gwbh->setText("过温保护中");
            ui->label_gwbh->setStyleSheet("background-color: rgb(225, 0, 0);");
        } else {
            ui->label_gwbh->setText("正常");
            ui->label_gwbh->setStyleSheet("background-color: rgb(85, 170, 127);");
        }
        if((UN_Cyc_InquireData.G_Cyc_InquireData.Erro & 0x10) == 16){
            ui->label_rldqy->setText("欠压保护中");
            ui->label_rldqy->setStyleSheet("background-color: rgb(225, 0, 0);");
        } else {
            ui->label_rldqy->setText("正常");
            ui->label_rldqy->setStyleSheet("background-color: rgb(85, 170, 127);");
        }
        if((UN_Cyc_InquireData.G_Cyc_InquireData.Erro & 0x20) == 32){
            ui->label_gfj->setText("异常");
            ui->label_gfj->setStyleSheet("background-color: rgb(225, 0, 0);");
        } else {
            ui->label_gfj->setText("正常");
            ui->label_gfj->setStyleSheet("background-color: rgb(85, 170, 127);");
        }
        if((UN_Cyc_InquireData.G_Cyc_InquireData.Erro & 0x40) == 64){
            ui->label_pt1000->setText("故障");
            ui->label_pt1000->setStyleSheet("background-color: rgb(225, 0, 0);");
        } else {
            ui->label_pt1000->setText("正常");
            ui->label_pt1000->setStyleSheet("background-color: rgb(85, 170, 127);");
        }
        if((UN_Cyc_InquireData.G_Cyc_InquireData.Erro & 0x80) == 128){
            ui->label_cd->setText("故障");
            ui->label_cd->setStyleSheet("background-color: rgb(225, 0, 0);");
        } else {
            ui->label_cd->setText("正常");
            ui->label_cd->setStyleSheet("background-color: rgb(85, 170, 127);");
        }

    }
    else if (mRec_Buf[OFFSET_CMD] == CMD_NORMAL_PARAMS)
    {
        memcpy(UN_Normal_SetDate.G_Normal_SetDate_8, mRec_Buf + SZ_OVERHEAD, mRec_Buf[OFFSET_LEN]);

        /* 锂电池参数设置 */
        // 锂电池可用Soc下限电压
        ui->lineEdit_lidiankeyongSocXiaX->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_SOCMin));

        // 锂电池可用Soc上限电压
        ui->lineEdit_lidiankeyongSocShangX->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_SOCMax));

        // 电堆充电触发Soc
        ui->lineEdit_startchongdianSoc->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_ChargeSoc));

        // 电堆停止充电截止Soc
        ui->lineEdit_stopchongdianSoc->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_StopSoc));

        // 电堆停止充电截止Soc
        ui->lineEdit_stopchongdianV->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_StopV, 'f', 1));

        // 电堆停止充电截止Soc
        ui->lineEdit_stopchongdianI->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_StopI, 'f', 1));

        // 锂电池安全下限电压
        ui->lineEdit_anqunVXiaX->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_Volmin, 'f', 1));

        // 充满换罐次数
        ui->lineEdit_LI_FullCharg_Cyc->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_FullCharg_Cyc));

        /* 氢罐参数设置 */
        // 氢罐初始容量
        ui->lineEdit_qingzongrongliang->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_IntCap));

        // 氢罐最低电量
        ui->lineEdit_qingzuidizhi->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_MinCap));

        // 清瓶模式时触发氢罐容量
        ui->lineEdit_qingqingpingrl->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_ClearCap_H));

        // 清瓶模式时触发锂电池容量
        ui->lineEdit_qingqingpingLDSOC->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_ClearCap_B));

        /* 电堆参数设置*/
        // 鼓风机最小pwm
        ui->lineEdit_gufengjipwmMin->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_PWM_min));

        // 鼓风机开机前运行时间
        ui->lineEdit_gfjKaiJiRuntime->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.BL_RunStartTim));

        // 电堆超温持续时间
        ui->lineEdit_dianduiOverTempertime->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.BL_OvertepTim));

        // 电堆吹扫持续时间
        ui->lineEdit_dianduiChiXuChuiFengtime->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.BL_HoldTim));

        // 燃料电池电压下限
        ui->lineEdit_ranliaoDCVMin->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_Vomin, 'f', 1));

        // 电堆电流下限
        ui->lineEdit_dianDuiDinliuMin->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_Iomin, 'f', 1));

        // 燃料电池温度上限
        ui->lineEdit_ranliaoTemperMAX->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_Tepmax, 'f', 1));

        // 燃料电池运行目标温度
        ui->lineEdit_ranliaoTargetTemper->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_TepTarge, 'f', 1));

        // 氢电池系数A
        ui->lineEdit_gufengjipwmA->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_PWM_A, 'f', 1));

        // 氢电池系数B
        ui->lineEdit_gufengjipwmB->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_PWM_B, 'f', 1));

        /* 排气参数 */
        // 间隔时间
        ui->lineEdit_paiqiJiangetime->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_Interval));

        // 排气时间-1
        ui->lineEdit_paiqitime_1->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_Time1, 'f', 1));

        // 排气时间-2
        ui->lineEdit_paiqitime_2->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_Time2, 'f', 1));

        // 电堆开机排氢次数
        ui->lineEdit_ddKaiJiPaiqingcnt->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_StartTimes));

        // 电堆堵水时功率预警值
        ui->lineEdit_dddushuiPower->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_WaterP));

        // 电堆堵水时排气阀循环数
        ui->lineEdit_ddduishuipaiqicnt->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_CycleTimes));

        // 风扇常开PWM
        ui->lineEdit_fanpwm->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Noropen_SetData.NoropenPWM_Fan));

        /* 鼓风机循环参数 */
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

        // 风扇常开PWM
        ui->lineEdit_blowerpwm->setText(QString::number(UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Noropen_SetData.NoropenPWM_Blower));

        show_PromptInformation("读取参数成功！");
    }
    else if (mRec_Buf[OFFSET_CMD] == CMD_NMOP_SWITCH)
    {
        if(mRec_Buf[OFFSET_DATA + OFFSET_ONE] == CLOSE && mRec_Buf[OFFSET_DATA + OFFSET_TWO] == SUCCEFUL){
            if(mRec_Buf[OFFSET_DATA] == DEVICE_BLOWER){
                show_PromptInformation("鼓风机关闭成功!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_FAN){
                show_PromptInformation("风扇关闭成功!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_VALVEIN){
                show_PromptInformation("进气阀门关闭成功!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_VALVEOUT){
                show_PromptInformation("出气阀门关闭成功!!!!");
            }
        } else if (mRec_Buf[OFFSET_DATA + OFFSET_ONE] == OPEN && mRec_Buf[OFFSET_DATA + OFFSET_TWO] == SUCCEFUL){
            if(mRec_Buf[OFFSET_DATA] == DEVICE_BLOWER){
                show_PromptInformation("鼓风机打开成功!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_FAN){
                show_PromptInformation("风扇打开成功!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_VALVEIN){
                show_PromptInformation("进气阀门打开成功!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_VALVEOUT){
                show_PromptInformation("出气阀门打开成功!!!!");
            }
        } else if (mRec_Buf[OFFSET_DATA + OFFSET_ONE] == CLOSE && mRec_Buf[OFFSET_DATA + OFFSET_TWO] == DEFEAT){
            if(mRec_Buf[OFFSET_DATA] == DEVICE_BLOWER){
                show_PromptInformation("鼓风机关闭失败!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_FAN){
                show_PromptInformation("风扇关闭失败!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_VALVEIN){
                show_PromptInformation("进气阀门关闭失败!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_VALVEOUT){
                show_PromptInformation("出气阀门关闭失败!!!!");
            }
        } else if (mRec_Buf[OFFSET_DATA + OFFSET_ONE] == OPEN && mRec_Buf[OFFSET_DATA + OFFSET_TWO] == DEFEAT){
            if(mRec_Buf[OFFSET_DATA] == DEVICE_BLOWER){
                show_PromptInformation("鼓风机打开失败!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_FAN){
                show_PromptInformation("风扇打开失败!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_VALVEIN){
                show_PromptInformation("进气阀门打开失败!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == DEVICE_VALVEOUT){
                show_PromptInformation("出气阀门打开失败!!!!");
            }
        }
    }
    else if (mRec_Buf[OFFSET_CMD] == CMD_MODE_SWITCH) {
        if(mRec_Buf[OFFSET_DATA + OFFSET_ONE] == CLOSE && mRec_Buf[OFFSET_DATA + OFFSET_TWO] == SUCCEFUL){
            if(mRec_Buf[OFFSET_DATA] == MODE_IMITATE){
                show_PromptInformation("维修模式关闭成功!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == MODE_MAINTAIN){
                show_PromptInformation("模拟模式关闭成功!!!!");
            }
        } else if (mRec_Buf[OFFSET_DATA + OFFSET_ONE] == OPEN && mRec_Buf[OFFSET_DATA + OFFSET_TWO] == SUCCEFUL){
            if(mRec_Buf[OFFSET_DATA] == MODE_IMITATE){
                show_PromptInformation("维修模式打开成功!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == MODE_MAINTAIN){
                show_PromptInformation("模拟模式打开成功!!!!");
            }
        } else if (mRec_Buf[OFFSET_DATA + OFFSET_ONE] == CLOSE && mRec_Buf[OFFSET_DATA + OFFSET_TWO] == DEFEAT){
            if(mRec_Buf[OFFSET_DATA] == MODE_IMITATE){
                show_PromptInformation("维修模式关闭失败!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == MODE_MAINTAIN){
                show_PromptInformation("模拟模式关闭失败!!!!");
            }
        } else if (mRec_Buf[OFFSET_DATA + OFFSET_ONE] == OPEN && mRec_Buf[OFFSET_DATA + OFFSET_TWO] == DEFEAT){
            if(mRec_Buf[OFFSET_DATA] == MODE_IMITATE){
                show_PromptInformation("维修模式打开失败!!!!");
            } else if (mRec_Buf[OFFSET_DATA] == MODE_MAINTAIN){
                show_PromptInformation("模拟模式打开失败!!!!");
            }
        }

    }
    else if (mRec_Buf[OFFSET_CMD] == CMD_RM_SETTING)
    {
        if(mRec_Buf[OFFSET_DATA + 14] == SUCCEFUL){
            memcpy(UN_Repair_InquireData.G_Repair_InquireData_8, mRec_Buf + SZ_OVERHEAD, mRec_Buf[OFFSET_LEN]);
            //维修模式下燃料电池温度上限
            ui->lineEdit_WXranliaoTemperMAX->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.HB_Tepmax, 'f', 1));
            //维修模式下燃料电池目标温度
            ui->lineEdit_WXranliaoTargetTemper->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.HB_TepTarge, 'f', 1));
            //鼓风机转速
            ui->lineEdit_GFJ_PWM->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.HB_BlowerDuty));
            //排气间隔时间
            ui->lineEdit_cycleTIM->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.EH_Interval));
            //排气时间1
            ui->lineEdit_Exhausttime_Y1->setText(QString::number(UN_Repair_InquireData.G_Cyc_InquireData.EH_Time1, 'f', 1));

            switch (mRec_Buf[OFFSET_DATA]) {
            case 0X01:
                show_PromptInformation("维修模式读取参数成功！");
                break;
            case 0X02:
                show_PromptInformation("维修模式设置参数成功！");
                break;
            case 0X03:
                show_PromptInformation("维修模式初始化参数成功！");
                break;
            }

        } else if(mRec_Buf[OFFSET_DATA + 14] == DEFEAT) {
            switch (mRec_Buf[OFFSET_DATA]) {
            case 0X01:
                show_PromptInformation("维修模式读取参数失败！");
                break;
            case 0X02:
                show_PromptInformation("维修模式设置参数失败！");
                break;
            case 0X03:
                show_PromptInformation("维修模式初始化参数失败！");
                break;
            }
        }

    }
    mRec_cnt = 0;
}

void Dialog::sendToKz(uint8_t cmd,uint8_t *buf,uint32_t len)
{
    if(len!=0)
    {
        memcpy(mSendpcbuf + SZ_OVERHEAD, buf, len);
    }
    mSendpcbuf[OFFSET_HEADER] = HEADER_FIRST_BYTE;
    mSendpcbuf[OFFSET_HEADER + 1] = HEADER_SECOND_BYTE;
    mSendpcbuf[OFFSET_SND_ADDR] = SWJADDR;//
    mSendpcbuf[OFFSET_RCV_ADDR] = XWJADDR;
    mSendpcbuf[OFFSET_CMD] = cmd;//cmd
    mSendpcbuf[OFFSET_LEN] = len; //长度
    mSendpcbuf[OFFSET_CRC] = checkcrc8(mSendpcbuf + OFFSET_CMD, len + SZ_CMD + SZ_LEN);
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
    this->setWindowIcon(QIcon(":/res/log2.png"));
    this->setWindowTitle("V0103_上位机");
    ui->label_log->setStyleSheet("QLabel {" "background-image: url(:/res/ql.png);" "border: none;" "}");
    ui->label_log->show();
}

void Dialog::SetParsing()
{
    if (mIsOpen)
    {
        /* 电堆参数设置 */
        // 氢电池输出电压下限
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_Vomin = ui->lineEdit_ranliaoDCVMin->text().toFloat();
        // 氢电池温度上限
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_Tepmax = ui->lineEdit_ranliaoTemperMAX->text().toFloat();
        // 氢电池电流下限
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_Iomin = ui->lineEdit_dianDuiDinliuMin->text().toFloat();
        // 氢电池目标温度
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_TepTarge = ui->lineEdit_ranliaoTargetTemper->text().toFloat();
        // 氢电池系数A
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_PWM_A = ui->lineEdit_gufengjipwmA->text().toFloat();
        // 氢电池系数B
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_PWM_B = ui->lineEdit_gufengjipwmB->text().toFloat();
        // 电堆超温持续时间
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.BL_OvertepTim = ui->lineEdit_dianduiOverTempertime->text().toUInt();
        // 电堆扫风持续时间
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.BL_HoldTim = ui->lineEdit_dianduiChiXuChuiFengtime->text().toUInt();
        // 鼓风机开机前运行时间
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.BL_RunStartTim = ui->lineEdit_gfjKaiJiRuntime->text().toUInt();
        // 鼓风机最小占空比
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HB_SetData.HB_PWM_min = ui->lineEdit_gufengjipwmMin->text().toUInt();

        /* 氢罐参数设置 */
        // 氢罐初始容量
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_IntCap = ui->lineEdit_qingzongrongliang->text().toUInt();
        // 氢罐最低电量
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_MinCap = ui->lineEdit_qingzuidizhi->text().toUInt();
        // 氢罐清罐氢罐容量
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_ClearCap_H = ui->lineEdit_qingqingpingrl->text().toUInt();
        // 氢罐清罐电池容量
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_HP_SetData.HP_ClearCap_B = ui->lineEdit_qingqingpingLDSOC->text().toUInt();

        /* 鼓风机循环参数 */
        // 第一次亏空时间
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_IntervalFirst = ui->lineEdit_BLCyc_IntervalFirst->text().toUInt();
        // 循环间隔时间
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_Interval = ui->lineEdit_gfjjiangetime->text().toUInt();
        // 开持续时间
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_OpenTim = ui->lineEdit_gfjchixutimeOpen->text().toUInt();
        // 关持续时间
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_CloseTim = ui->lineEdit_gfjchixutimeClose->text().toUInt();
        // 启停次数
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Blower_SetData.BLCyc_SSTimes = ui->lineEdit_gfjyunxingcishu->text().toUInt();
        // 鼓风机常开PWM
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Noropen_SetData.NoropenPWM_Blower = ui->lineEdit_blowerpwm->text().toUInt();

        /* 排气参数 */
        // 排气间隔时间
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_Interval = ui->lineEdit_paiqiJiangetime->text().toUInt();
        // 排气时间-1
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_Time1 = ui->lineEdit_paiqitime_1->text().toFloat();
        // 排气时间-2
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_Time2 = ui->lineEdit_paiqitime_2->text().toFloat();
        // 堵水功率
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_WaterP = ui->lineEdit_dddushuiPower->text().toUInt();
        // 开机排气次数
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_StartTimes = ui->lineEdit_ddKaiJiPaiqingcnt->text().toUInt();
        // 堵水排气次数
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Exhaust_SetData.EH_CycleTimes = ui->lineEdit_ddduishuipaiqicnt->text().toUInt();
        // 鼓风机常开PWM
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Noropen_SetData.NoropenPWM_Fan = ui->lineEdit_fanpwm->text().toUInt();

        /* 锂电池参数 */
        // 锂电池soc上限电压
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_SOCMax = ui->lineEdit_lidiankeyongSocShangX->text().toUInt();
        // 锂电池soc下限电压
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_SOCMin = ui->lineEdit_lidiankeyongSocXiaX->text().toUInt();
        // 充电触发Soc
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_ChargeSoc = ui->lineEdit_startchongdianSoc->text().toUInt();
        // 充电截止Soc
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_StopSoc = ui->lineEdit_stopchongdianSoc->text().toUInt();
        // 充电截止Soc
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_StopV = ui->lineEdit_stopchongdianV->text().toFloat();
        // 充电截止Soc
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_StopI = ui->lineEdit_stopchongdianI->text().toFloat();
        // 锂电池电压下限
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_Volmin = ui->lineEdit_anqunVXiaX->text().toFloat();
        // 满充换罐次数
        UN_Normal_SetDate.G_Normal_SetDate.G_pUp_Battery_SetData.LI_FullCharg_Cyc = ui->lineEdit_LI_FullCharg_Cyc->text().toUInt();

        uint8_t buf[sizeof(UN_Normal_SetDate)] = { 0 };
        memcpy(buf, &UN_Normal_SetDate, sizeof(UN_Normal_SetDate));
        sendToKz(CMD_NORMAL_PARAMS, buf, sizeof(UN_Normal_SetDate));
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
    QByteArray recvData = mSerialPort->readAll();
    char *pdata = recvData.data();
    uint16_t bufferLen = recvData.size();
    qDebug() << "SerialPort接收的数据11111111111111111111111111111111111111111111111111";
    for (uint16_t i = 0; i < bufferLen; ++i)
        qDebug("<0x%x>", *(uint8_t *)(pdata + i));
    qDebug() << "SerialPort接收的数据22222222222222222222222222222222222222222222222222";
    qDebug() << "buffer.size = " << bufferLen;
    if (!memcmp(headerArray, pdata, sizeof(headerArray))){	//比较相同返回0
        memcpy(mRec_Buf, pdata, bufferLen);
        ParsingData();
        return;
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

void Dialog::periodicQuery()
{
    uint8_t buf[1] = { 0 };
    buf[0] = 0XAB;
    sendToKz(CMD_PERODIC_QUERY, buf, 1);
    periodicQueryTimer->start(PERIODIC_QUERY_TIME);
}
