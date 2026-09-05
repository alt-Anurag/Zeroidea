#include <QTRSensors.h>
#include "TB6612FNG.h"
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define AIN1 21
#define BIN1 25
#define AIN2 22
#define BIN2 33
#define PWMA 23
#define PWMB 32
#define STBY 19

const int offsetA = 1;
const int offsetB = 1;

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA);
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB);

QTRSensors qtr;
BluetoothSerial SerialBT;

const uint8_t SensorCount = 5;
uint16_t sensorValues[SensorCount];
int threshold[SensorCount];

float Kp = 0;
float Ki = 0;
float Kd = 0;

uint8_t multiP = 1;
uint8_t multiI = 1;
uint8_t multiD = 1;
float Pvalue;
float Ivalue;
float Dvalue;

boolean onoff = false;

int val, cnt = 0, v[3];

uint16_t position;
int P, D, I, previousError, PIDvalue, error;
int lsp, rsp;
int lfspeed = 230;

void setup()
{
    pinMode(STBY, OUTPUT);
    digitalWrite(STBY, HIGH);

    qtr.setTypeAnalog();
    qtr.setSensorPins((const uint8_t[]){26, 27, 14, 12, 13}, SensorCount);

    delay(500);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.begin(115200);
    SerialBT.begin("LFR_Robot");
    Serial.println("Bluetooth Started! Ready to pair...");

    for (uint16_t i = 0; i < 400; i++)
    {
        qtr.calibrate();
    }
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Sensor thresholds:");
    for (uint8_t i = 0; i < SensorCount; i++)
    {
        threshold[i] = (qtr.calibrationOn.minimum[i] + qtr.calibrationOn.maximum[i]) / 2;
        Serial.print(threshold[i]);
        Serial.print("  ");
    }
    Serial.println();

    delay(1000);
}

void loop()
{
    if (SerialBT.available())
    {
        while (SerialBT.available() > 0)
        {
            valuesread();
            processing();
        }
    }

    if (onoff == true)
    {
        robot_control();
    }
    else if (onoff == false)
    {
        motor1.brake();
        motor2.brake();
    }
}

void robot_control()
{
    position = qtr.readLineBlack(sensorValues);
    error = 2000 - position;

    if (sensorValues[0] >= 980 && sensorValues[1] >= 980 && sensorValues[2] >= 980 && sensorValues[3] >= 980 && sensorValues[4] >= 980)
    {
        if (previousError > 0)
        {
            motor_drive(-230, 230);
        }
        else
        {
            motor_drive(230, -230);
        }
        position = qtr.readLineBlack(sensorValues);
    }
    else
    {
        PID_Linefollow(error);
    }
}

void PID_Linefollow(int error)
{
    P = error;
    I = I + error;
    D = error - previousError;

    Pvalue = (Kp / pow(10, multiP)) * P;
    Ivalue = (Ki / pow(10, multiI)) * I;
    Dvalue = (Kd / pow(10, multiD)) * D;

    float PIDvalue = Pvalue + Ivalue + Dvalue;
    previousError = error;

    lsp = lfspeed - PIDvalue;
    rsp = lfspeed + PIDvalue;

    if (lsp > 255) lsp = 255;
    if (lsp < -255) lsp = -255;
    if (rsp > 255) rsp = 255;
    if (rsp < -255) rsp = -255;

    motor_drive(lsp, rsp);
}

void valuesread()
{
    val = SerialBT.read();
    cnt++;
    v[cnt] = val;
    if (cnt == 2)
        cnt = 0;
}

void processing()
{
    int a = v[1];
    switch (a)
    {
        case 1: Kp = v[2]; break;
        case 2: multiP = v[2]; break;
        case 3: Ki = v[2]; break;
        case 4: multiI = v[2]; break;
        case 5: Kd = v[2]; break;
        case 6: multiD = v[2]; break;
        case 7: onoff = v[2]; break;
    }
}

void motor_drive(int left, int right)
{
    if (right >= 0)
    {
        motor2.drive(right);
    }
    else
    {
        motor2.drive(right);
    }

    if (left >= 0)
    {
        motor1.drive(left);
    }
    else
    {
        motor1.drive(left);
    }
}
