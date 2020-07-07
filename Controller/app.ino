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
#define REMOTE_PWR 0xFFA25D
#define REMOTE_VOL_UP 0xFF629D
#define REMOTE_FUNC_STOP 0xFFE21D
#define REMOTE_PREV 0xFF22DD
#define REMOTE_PAUSE_PLAY 0xFF02FD
#define REMOTE_NEXT 0xFFC23D
#define REMOTE_DOWN 0xFFE01F
#define REMOTE_VOL_DOWN 0xFFA857
#define REMOTE_UP 0xFF906F
#define REMOTE_0 0xFF6897
#define REMOTE_EQ 0xFF9867
#define REMOTE_ST_REPT 0xFFB04F
#define REMOTE_1 0xFF30CF
#define REMOTE_2 0xFF18E7
#define REMOTE_3 0xFF7A85
#define REMOTE_4 0xFF10EF
#define REMOTE_5 0xFF38C7
#define REMOTE_6 0xFF5AA5
#define REMOTE_7 0xFF42BD
#define REMOTE_8 0xFF4AB5
#define REMOTE_9 0xFF52AD
#define REMOTE_REPEAT 0xFFFFFFFF
#define clamp(X, Y, N) (N > Y ? Y : N < X ? X : N)

enum Command : uint8_t
{
  TogglePower
};
IRrecv irrecv(MOTION_PIN);
decode_results results;
unsigned long time_now = 0;

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
    if (irrecv.decode(&results))
    {
      Serial.println(results.value, HEX);
      irrecv.resume();
    }
    time_now = millis();
  }
}