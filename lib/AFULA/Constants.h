#ifndef Constants_h
#define Constants_h

//components
#define BLINKER_BLUE_LED D8
#define BLINKER_RED_LED D7
#define SIREN_SPEAKER D2
#define FLAME_SENSOR A0
#define GSM_IO_1 D5
#define GSM_IO_2 D6

//motor commands
#define FORWARD 'F'
#define BACKWARD 'B'
#define RIGHT 'R'
#define LEFT 'L'
#define STOP 'S'
#define EXTINGUISH 'E'
#define RESET_MOTOR 'X'
#define FLAME_IR 'I'
#define FLAME_IR_MOTOR 'M'

//gsm
#define SMS_RECIPIENT "+639065531651"
#define SMS_MODE "AT+CMGF=1"
#define SMS_RECIPIENT_COMMAND ("AT+CMGS=\"" SMS_RECIPIENT "\"\r")
#define SMS_END 26

//wifi
#define SSID "arduino_net"
#define PASSWORD "arduino_net"
#define AI_SERVER "http://192.168.1.110"
#define SSID_FALLBACK "PLDTHOMEFIBRTx4z7"
#define PASSWORD_FALLBACK "HindiKoAlamE1Ko!"
#define AI_SERVER_FALLBACK "http://192.168.1.216"

//settings
#define TEST_MODE false
#define NO_LOGS true
#define MOTOR_OFF false
#define MANUAL_DRIVE_MODE false
#define COMMAND_TIMER 0
#define COMMAND_RESET_TIMER 100
#define SETUP_COUNTER 0
#define MUSIC_MODE false

#endif