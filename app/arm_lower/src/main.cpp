#include "mbed.h"
#include "rover_config.h"
#include "CANMsg.h"
#include "PwmIn.h"
#include "PID.h"
#include "Motor.h"

const unsigned int  RX_ID = ROVER_ARM_LOWER_CANID; 
const unsigned int  TX_ID = ROVER_JETSON_CANID; 
const unsigned int  CAN_MASK = ROVER_CANID_FILTER_MASK;

const float         VELOCITY_TO_PWM_DUTY_SCALER = 5.0;

bool directMotorControlEnabled = true;

double jointAngle[3];

Serial              pc(SERIAL_TX, SERIAL_RX, ROVER_DEFAULT_BAUD_RATE);
CAN                 can(CAN_RX, CAN_TX, ROVER_CANBUS_FREQUENCY);
CANMsg              rxMsg;
CANMsg              txMsg;

DigitalOut          ledErr(LED1);
DigitalOut          ledCAN(LED4);

Motor               turnTableMotor(MOTOR1, MOTOR1_DIR, ROVER_MOTOR_PWM_FREQ_HZ, true);
Motor               shoulderMotor(MOTOR2, MOTOR2_DIR, ROVER_MOTOR_PWM_FREQ_HZ);
Motor               elbowMotor(MOTOR3, MOTOR3_DIR, ROVER_MOTOR_PWM_FREQ_HZ);
PwmIn               absEnc1(ENC_A1);
PwmIn               absEnc2(ENC_A2);
PwmIn               absEnc3(ENC_A3);

Timer               canSendTimer; 

void printCANMsg(CANMessage& msg) {
    pc.printf("  ID      = 0x%.3x\r\n", msg.id);
    pc.printf("  Type    = %d\r\n", msg.type);
    pc.printf("  Format  = %d\r\n", msg.format);
    pc.printf("  Length  = %d\r\n", msg.len);
    pc.printf("  Data    =");
    for(int i = 0; i < msg.len; i++)
        pc.printf(" 0x%.2X", msg.data[i]);
    pc.printf("\r\n");
}

enum armCommand {
 
    configureDirectMotorControl = RX_ID,
    setTurnTableSpeed,
    setShoulderSpeed,
    setElbowSpeed,

    firstCommand = configureDirectMotorControl,
    lastCommand  = setElbowSpeed

};

void initCAN() {
    can.filter(RX_ID, ROVER_CANID_FILTER_MASK, CANStandard);

    // for (int canHandle = firstCommand; canHandle <= lastCommand; canHandle++) {
    //     can.filter(RX_ID + canHandle, 0xFFF, CANStandard, canHandle);
    // }
}

float handlerSetSpeedMotor(Motor *motor, CANMsg *motorSpeedMsg) {
    if (directMotorControlEnabled) {
        double setSpeed = 0.0;
        *motorSpeedMsg >> setSpeed;
        setSpeed /= VELOCITY_TO_PWM_DUTY_SCALER;
        motor->speed(setSpeed);

        return setSpeed;
    }

    else {
        ledErr = 1;
        pc.printf("ERROR: Direct motor control disabled\r\n");

        return 0.0;
    }
}

void proccessCANMsg(CANMsg *p_newMsg) {
    switch (p_newMsg->id) {
        // case configureDirectMotorControl: 
        //     pc.printf("\r\nRecieved command configureDirectMotorControl\r\n");

        //     *newMsg >> directMotorControlEnabled;

        //     if (directMotorControlEnabled) {
        //         pc.printf("Enabled directMotorControl\r\n");
        //     }
        //     else {   
        //         pc.printf("Disabled directMotorControl\r\n");
        //     }    

        //     break;

        case setTurnTableSpeed:
            pc.printf("\r\nRecieved command setTurnTableSpeed\r\n");
            pc.printf("Set the speed of motor 1 to %f\r\n", handlerSetSpeedMotor(&turnTableMotor, p_newMsg));

            break;

        case setShoulderSpeed:
            pc.printf("\r\nRecieved command setShoulderSpeed\r\n");
            pc.printf("Set the speed of motor 2 to %f\r\n", handlerSetSpeedMotor(&shoulderMotor, p_newMsg));

            break;

        case setElbowSpeed:
            pc.printf("\r\nRecieved command setElbowSpeed\r\n");
            pc.printf("Set the speed of motor 3 to %f\r\n", handlerSetSpeedMotor(&elbowMotor, p_newMsg));

            break;

        default:
            pc.printf("Recieved unimplemented command\r\n");

            break;
    }
}

// void updateJointAngles() { 
//     jointAngle[0] = absEnc1.avgDutyCycle() * 360.0; 
//     jointAngle[1] = absEnc2.avgDutyCycle() * 360.0; 
//     jointAngle[2] = absEnc3.avgDutyCycle() * 360.0; 
// } 
 
// void sendJointAnglesToJetson() { 

//     for (int i = 0; i < 3; i++) { 
//         double a = jointAngle[i];
//         char arr[sizeof(a)];
//         memcpy(arr,&a,sizeof(a));

//         CANMsg txMsg(TX_ID, arr, sizeof(arr));
         
//         if(can.write(txMsg)) { 
//             // pc.printf("Sent joint %d angle to jetson\r\n", i); 
//         } 
//         else {
//             pc.printf("ERROR: CAN send error!\r\n");
//         }
//     } 
// } 
 
int main(void)
{
    pc.printf("Program Started\r\n\r\n");

    initCAN();

    canSendTimer.start();

    while (1) {

        if (can.read(rxMsg)) {
            proccessCANMsg(&rxMsg);
            rxMsg.clear();
            ledCAN = !ledCAN;
        }

        // updateJointAngles();

        // if (canSendTimer.read() > 0.1) {
        //     sendJointAnglesToJetson();
        //     canSendTimer.reset();
        // }

    }
}
 
