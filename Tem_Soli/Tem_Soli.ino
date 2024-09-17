#include <Wire.h>

/********************宏定义上传时间***************************/
#define uptime 1 //上传时间 单位秒；
/********************宏定义***************************/


/********************温湿度传感器***************************/
char senddata[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
char receivedata[9] = {0x00};

/********************土壤传感器***************************/
char soilsenddata[8] = {0x02,0x03,0x00,0x00,0x00,0x04,0x44,0x3A};
char soilreceivedata[13] ={0x00};

/********************温湿度加土壤***************************/
float hum,atem,solitemp,solihum,soilconductivity,soilPH;

/********************中间变量（温度低于0度是以补码形式）***************************/
int16_t tem,soiltem;


/********************tmp117传感器初始化变量以及对应函数***************************/
/*
 * TMP117传感器
 * */
uint8_t Tmp117Addr[4] = {0x48, 0x49, 0x4A, 0x4B};
float temperature[4];
float lastValidTemperature[4] = {0.0, 0.0, 0.0, 0.0};  // 存储最后一次有效的温度读数
bool deviceOnline[4] = {false, false, false, false};  // 用于存储设备在线状态
bool isFirstCheck = true;  // 指示是否为第一次检查设备状态
char r1[256]; // 修改为足够大的数组以容纳 JSON 字符串
// 定义休眠时间间隔（以微秒为单位）
const uint64_t INTERVAL = 60000000; // 一分钟，单位为微秒

void TMP117_Write_Byte(uint8_t addr, uint8_t reg, uint16_t data) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(data >> 8);
    Wire.write(data & 0xFF);
    Wire.endTransmission();
}

uint16_t TMP117_Read_Byte(uint8_t addr, uint8_t reg) {
    uint16_t temp16 = 0;

    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((int) addr, 2);
    if (Wire.available() == 2) {
        temp16 = Wire.read();
        temp16 = (temp16 << 8) | Wire.read();
    }
    return temp16;
}

float TMP117_Read_Temperature(uint8_t addr) {
    uint16_t data = TMP117_Read_Byte(addr, 0x00);  // Assume 0x00 is the temperature register
    if (data < 0x8000) {  // Positive temperature
        return data * 0.0078125;
    } else {  // Negative temperature
        return (data - 65536) * 0.0078125;
    }
}

/********************tmp117传感器初始化变量以及对应函数***************************/


/********************主函数启动部分（执行一次）***************************/
void setup() {
    Wire.begin();
//    Wire.setClock(30000);
    Serial.begin(9600);
    Serial2.begin(9600);
/********************判断TMP117是否连接成功***************************/
    if (isFirstCheck) {
        for (int i = 0; i < 4; i++) {
            Wire.beginTransmission(Tmp117Addr[i]);
            deviceOnline[i] = (Wire.endTransmission() == 0);
            if (!deviceOnline[i]) {
                Serial.print("I2C device at address 0x");
                Serial.print(Tmp117Addr[i], HEX);
                Serial.println(" offline.");
            }
        }
        isFirstCheck = false;
    }
    delay(100);
/********************判断TMP117是否连接成功***************************/
}
/********************主函数执行部分（循环执行）***************************/
void loop() {
  int j =0;
  int k =0;
    // esp_sleep_enable_timer_wakeup(INTERVAL);
    
/********************tmp117传感器***************************/
    for (int i = 0; i < 4; i++) {
        if (deviceOnline[i]) {
            temperature[i] = TMP117_Read_Temperature(Tmp117Addr[i]);
            if (temperature[i] > 150 || temperature[i] < -50) {
                temperature[i] = 99;  // Indicate an error or unrealistic reading
            }
        } else {
            temperature[i] = -99;  // Indicate device is offline
        }
    }
    delay(100);
/********************tmp117传感器***************************/

/********************空气传感器***************************/
    Serial2.write(senddata,8);
    delay(100);
    while(Serial2.available()>0)
    {
        
        char receivedChar = Serial2.read();
        receivedata[j++] = receivedChar;
    }

    hum =( receivedata[3] * 256 + receivedata[4] ) * 0.1;
    
    tem =( receivedata[5] * 256 + receivedata[6] ) ;   
    if(tem & 0x8000){
      tem = tem - 0x10000 ;
      }
    atem = tem * 0.1;
      
    delay(100);
/********************空气传感器***************************/

/********************土壤传感器***************************/
    Serial2.write(soilsenddata,8);
    delay(100);
    while(Serial2.available()>0)
    {
        
        char receivedsoilChar = Serial2.read();
        soilreceivedata[k++] = receivedsoilChar;
    }
//    Serial.write(soilreceivedata, 13);
    solihum = ( soilreceivedata[3] * 256 + soilreceivedata[4] ) * 0.1;
    soilconductivity = ( soilreceivedata[7] * 256 + soilreceivedata[8] ) ;
    soilPH = ( soilreceivedata[9] * 256 + soilreceivedata[10] ) * 0.1;

     soiltem = ( soilreceivedata[5] * 256 + soilreceivedata[6] ) ;
     if(soiltem & 0x8000){
      soiltem = soiltem - 0x10000 ;
      }
    solitemp = soiltem * 0.1;
    
    delay(200);
/********************土壤传感器***************************/  


    snprintf(r1, sizeof(r1), "{\"t1\":%.2f,\"t2\":%.2f,\"t3\":%.2f,\"atem\":%.2f,\"ahum\":%.2f,\"soiltem\":%.2f,\"soilhum\":%.2f,\"soilconductivity\":%.0f,\"soilPH\":%.2f}", temperature[0], temperature[1], temperature[2], atem,hum,solitemp,solihum,soilconductivity,soilPH);
    Serial.write(r1, strlen(r1)); // 发送 JSON 字符串到串行端口
    delay(1000* uptime); // 添加延迟以避免过度发送数据
    // esp_deep_sleep_start();
}

