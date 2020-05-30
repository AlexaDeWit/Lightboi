#include <boarddefs.h>
#include <ir_Lego_PF_BitStreamEncoder.h>
#include <IRremote.h>
#include <IRremoteInt.h>

#define NUM_LEDS 300
#define DATA_PIN 6
#define ACTIVE_LEDS 4
#define PERIOD_MS 10
#define MOTION_PIN 2
#define IR_PIN 8
#define REMOTE_PWR 0xFD00FF
#define REMOTE_VOL_UP 0xFD807F
#define REMOTE_FUNC_STOP 0xFD40BF
#define REMOTE_PREV 0xFD20DF
#define REMOTE_PAUSE_PLAY 0xFDA05F
#define REMOTE_NEXT 0xFD609F
#define REMOTE_DOWN 0xFD10EF
#define REMOTE_VOL_DOWN 0xFD906F
#define REMOTE_UP 0xFD50AF
#define REMOTE_0 0xFD30CF
#define REMOTE_EQ 0xFDB04F
#define REMOTE_ST_REPT 0xFD708F
#define REMOTE_1 0xFD08F7
#define REMOTE_2 0xFD8877
#define REMOTE_3 0xFD48B7
#define REMOTE_4 0xFD28D7
#define REMOTE_5 0xFDA857
#define REMOTE_6 0xFD6897
#define REMOTE_7 0xFD18E7
#define REMOTE_8 0xFD9867
#define REMOTE_9 0xFD58A7
#define clamp(X, Y, N) (N > Y ? Y : N < X ? X : N)

enum Command : uint8_t
{
  TogglePower
};

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn();
  pinMode(MOTION_PIN, INPUT);
}

void loop()
{
  if (millis() - time_now > PERIOD_MS)
  {
    time_now = millis();
  }
}