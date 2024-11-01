#ifndef PARAM_H
#define PARAM_H

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef struct
{

    float HB_Voltage;     //反应堆电压
    float HB_Current;     //反应堆电流
    float HB_Power;       //反应堆功率
    float HB_Temper;      //反应堆温度
    float HB_AllRunTime;  //反应堆累计运行时间 小时
    uint16_t HB_AllCap;   //反应堆氢气总容量
    uint16_t HB_FanSpeed; //扇热风扇转速/分钟
    uint16_t HB_BlowerSpeed;  //鼓风机转速
    uint8_t  HB_FanDuty;      //风扇占空比
    uint8_t  HB_BlowerDuty;   //鼓风机占空比
    float Li_Voltage;	  //锂电池电压
    float Li_Current;	  //锂电池电流
    float Li_Power;	         //锂电池功率
    float Li_Temper;		  //锂电池温度
    float Li_RunTime;	  //锂电池累计运行时间  小时
    float HJ_Outpower;	  //氢罐输出电量
    uint8_t  Li_SOC;	         //锂电池SOC
    uint8_t  Erro;            //故障信息
} p_Cyc_InquireData;  //循环查询数据


typedef struct
{

    float HB_Vomin;     //氢电池输出电压下限
    float HB_Tepmax;     //氢电池温度上限
    float HB_Iomin;       //氢电池电流下限
    float HB_TepTarge;      //氢电池目标温度
    float HB_PWM_A;         //氢电池系数A
    float HB_PWM_B;         //氢电池系数B
    uint16_t BL_OvertepTim;   //电堆超温持续时间
    uint16_t BL_HoldTim;      //电堆扫风持续时间
    uint16_t BL_RunStartTim;  //鼓风机开机前运行时间
    uint16_t HB_PWM_min;      //鼓风机最小占空比

} pUp_HB_SetData;  //氢堆参数设置  32byte

typedef struct
{
    uint16_t HP_IntCap;       //氢罐初始容量
    uint16_t HP_MinCap;       //氢罐最低容量
    uint16_t HP_ClearCap_H;   //氢罐清罐氢罐容量
    uint16_t HP_ClearCap_B;      //氢罐清罐电池容量

} pUp_HP_SetData;  //氢罐参数设置  8byte

typedef struct
{
    uint16_t BLCyc_IntervalFirst;       //第一次亏空时间
    uint16_t BLCyc_Interval;            //循环间隔时间
    uint16_t BLCyc_OpenTim;             //开持续时间
    uint16_t BLCyc_CloseTim;            //关持续时间
    uint16_t BLCyc_SSTimes;             //启停次数
} pUp_Blower_SetData;  //鼓风机循环参数设置  10byte

typedef struct
{
    uint16_t EH_Interval;          //排气间隔时间
    uint16_t EH_Time1;             //排气时间1
    uint16_t EH_Time2;             //排气时间2
    uint16_t EH_WaterP;            //堵水功率
    uint8_t EH_StartTimes;         //开机排气次数
    uint8_t EH_CycleTimes;         //堵水排气次数
} pUp_Exhaust_SetData;  //排气循环参数设置  10byte


typedef struct
{
    uint8_t LI_SOCMax;                //锂电池SOC上限
    uint8_t LI_SOCMin;                //锂电池SOC下限
    uint8_t LI_ChargeSoc;             //充电触发SOC
    uint8_t LI_StopSoc;               //充电截止SOC
    float LI_StopV;				  //充电截止电压
    float LI_StopI;				  //充电截止电流
    float LI_Volmin;                  //锂电池电压下限
    uint8_t LI_FullCharg_Cyc;         //满充换罐次数
} pUp_Battery_SetData;  //电池参数设置  9byte

typedef struct
{
    uint8_t NoropenPWM_Blower;        //鼓风机常开PWM
    uint8_t NoropenPWM_Fan;           //风扇常开PWM
} pUp_Noropen_SetData;  //pwm参数设置  2byte

typedef struct
{
    uint16_t EH_Interval;          //维修模式下排气间隔时间
    uint16_t EH_Time1;             //维修模式下排气时间1
    float HB_Tepmax;               //维修模式下燃料电池温度上限
    float HB_TepTarge;             //维修模式下燃料电池目标温度
    uint8_t HB_BlowerDuty;         //维修模式下鼓风机转速
} pUp_Repair_SetData;  //维修参数设置  13byte

union
{
    uint8_t G_Normal_SetDate_8[(sizeof(pUp_HB_SetData)+ sizeof(pUp_HP_SetData)
             +sizeof(pUp_Blower_SetData) +sizeof(pUp_Exhaust_SetData)
                 + sizeof(pUp_Battery_SetData) + sizeof (pUp_Noropen_SetData))];   // size = 69
    struct
    {
    pUp_HB_SetData        G_pUp_HB_SetData;  //电堆参数设置  32byte
    pUp_HP_SetData        G_pUp_HP_SetData;  //氢罐参数设置  8byte
    pUp_Blower_SetData    G_pUp_Blower_SetData; //鼓风机循环参数设置  10byte
    pUp_Exhaust_SetData   G_pUp_Exhaust_SetData; //排气循环参数设置  10byte
    pUp_Battery_SetData   G_pUp_Battery_SetData; //电池参数设置  9byte
    pUp_Noropen_SetData   G_pUp_Noropen_SetData;
    }
    G_Normal_SetDate;
}UN_Normal_SetDate;   //普通设置参数


union
{
    uint8_t G_Cyc_InquireData_8[sizeof(p_Cyc_InquireData)];  //size = 54
    p_Cyc_InquireData  G_Cyc_InquireData;
}UN_Cyc_InquireData;   //循环查询数据

union
{
    uint8_t G_Repair_InquireData_8[sizeof(pUp_Repair_SetData)]; //size = 13
    pUp_Repair_SetData  G_Cyc_InquireData;
}UN_Repair_InquireData;   //维修数据
#endif // PARAM_H
