#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QSerialPort>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QTimer>
#include <QPalette>

#define SZ_HEADER 2
#define SZ_SWJADDR 1
#define SZ_XWJADDR 1
#define SZ_ADDR 1
#define SZ_CRC 1
#define SZ_CMD 1
#define SZ_LEN 1
#define SZ_OVERHEAD  		(SZ_HEADER + SZ_SWJADDR + SZ_XWJADDR + SZ_CRC + SZ_CMD + SZ_LEN)

#define SWJADDR				0x15
#define XWJADDR				0x2A
#define HEADER_FIRST_BYTE	0XAA
#define HEADER_SECOND_BYTE	0X55

#define OFFSET_HEADER		0
#define OFFSET_SND_ADDR 	SZ_HEADER
#define OFFSET_RCV_ADDR 	(SZ_HEADER + SZ_ADDR)
#define OFFSET_CRC 			(SZ_HEADER + 2 * SZ_ADDR)
#define OFFSET_CMD 			(OFFSET_CRC + SZ_CRC)
#define OFFSET_LEN 			(OFFSET_CMD + SZ_CMD)
#define OFFSET_DATA 		(OFFSET_LEN + SZ_LEN)
#define OPEN                0X01
#define CLOSE               0X00

#define PERIODIC_QUERY_TIME 1000 // unit ms

enum {
    CMD_PERODIC_QUERY    = 0XA0,
    CMD_NORMAL_PARAMS    = 0XA1,
    CMD_NMOP_SWITCH      = 0XA2, // normally open switch
    CMD_MODE_SWITCH      = 0XA3,
    CMD_RM_SETTING       = 0XA4, // repair mode setting
    CMD_NORMALPARAM_READ = 0XA5,
};

enum {
    DEVICE_BLOWER   = 0X01,
    DEVICE_FAN      = 0X04,
    DEVICE_VALVEIN  = 0X08,
    DEVICE_VALVEOUT = 0X0F,
};

enum {
    MODE_IMITATE  = 0X01,
    MODE_MAINTAIN = 0X02,
};

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

    // 获取串口配置
    void getSerialPortConfig(void);
    // 设置串口配置
    bool setSerialPortConfig(void);

    unsigned char checkcrc8(uint8_t * buf ,uint32_t len);

    unsigned char checkcrcBA(QByteArray buf ,int len);

    void sendToKz(uint8_t cmd,uint8_t *buf,uint32_t len);

    void allInit(void);

    void show_PromptInformation_Red(QString str);

     void send_Data(uint8_t *buf, uint8_t len);

     float uint32ToFloat(uint32_t intValue);


private slots:
    void periodicQuery();

    void on_BtnOpen_clicked();

    void UpdatePort();

    // 清理提示框信息
    void show_PromptInformation(QString str);

    // 解析接收数据
    void ParsingData();
    // 设置参数
    void SetParsing();
    // 维修模式设置参数
    void WXSetParsing();

    void SaveData();

    void RecData();

private:
    Ui::Dialog *ui;

    bool mIsOpen;
    bool mHexShowFlag;
    bool mRec_Flag;
    bool mWeixiu_Flag;
    bool mGuFen_Flag;
    bool mFenSan_Flag;
    bool mQingIn_Flag;
    bool mQingOut_Flag;
    bool mMoNi_Flag;
    bool mWrite_Flag;
    bool mQingOut_Flag2;
    bool mQingIn_Flag2;
    bool mSave_Flag;

    QSerialPort *mSerialPort;
    QString mPortName;
    QString mBaudRate;
    QString mParity;
    QString mDataBits;
    QString mStopBits;
    QStringList oldPortStringList;
    QByteArray mRecData;
    QByteArray mMyData2;
    QByteArray mReadCmd;
    uint8_t mRec_Buf[100] = {0};
    uint8_t mRec_cnt = 0;
    uint8_t mSendBuf[100];
    uint8_t mSendCmd[100];
    uint8_t mSendpcbuf[200];
    QTimer *mSaveTimer;
    QTimer *periodicQueryTimer;
    QString mTemp_Flag;
    QString mV_Flag;
    QString mQing_Flag;
    QPalette mPal;
    QString mSavePath;
    QString mPathName;

    uint8_t mZKUpData[200];
    uint8_t mPCWriteData[200];
    uint8_t mPCReadData[200];

};

typedef struct {
        unsigned char headerOne;
        unsigned char headerTwo;
        unsigned char headerThree;
        unsigned char herderFour;
        unsigned char cmd;
        unsigned char length;
        unsigned char tempCmd;
        unsigned char highTemp;
        unsigned char lowTemp;
} cmdTemp;

#endif // DIALOG_H
