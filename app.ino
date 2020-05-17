#include <boarddefs.h>
#include <ir_Lego_PF_BitStreamEncoder.h>
#include <IRremote.h>
#include <IRremoteInt.h>

#include <bitswap.h>
#include <chipsets.h>
#include <color.h>
#include <colorpalettes.h>
#include <colorutils.h>
#include <controller.h>
#include <cpp_compat.h>
#include <dmx.h>
#include <fastled_config.h>
#include <fastled_delay.h>
#include <fastled_progmem.h>
#include <FastLED.h>
#include <fastpin.h>
#include <fastspi_bitbang.h>
#include <fastspi_dma.h>
#include <fastspi_nop.h>
#include <fastspi_ref.h>
#include <fastspi_types.h>
#include <fastspi.h>
#include <hsv2rgb.h>
#include <led_sysdefs.h>
#include <lib8tion.h>
#include <noise.h>
#include <pixelset.h>
#include <pixeltypes.h>
#include <platforms.h>
#include <power_mgt.h>

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

CRGB leds[NUM_LEDS];
IRrecv irrecv(IR_PIN);
decode_results ir_results;
unsigned long time_now = 0;

void turnOffLeds()
{
  for (int16_t i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }
};

class IState
{
public:
  virtual void Update(int deltaMs);
  virtual void ReceiveCommand(Command command);
};

class PowerState : public IState
{
public:
  virtual bool IsSuspended();
  virtual IState *ChildProgram();
};

class OffState : public IState
{
  OffState()
  {
  }
  ~OffState()
  {
  }

  void Update(int deltaMs)
  {
    //NOOP for now
  }
  void ReceiveCommand(Command command)
  {
    //NOOP for now
  }
};

enum LightProgram : unsigned char
{
  Purple_Breath
};

#define BREATH_CYCLE 5000
#define BREATH_STEPS_PER_CYCLE 100
#define BREATH_CYCLE_TIME 50
class Breath : public IState
{
  uint16_t CycleCount;
  uint16_t Time;
  bool increasing;
  CRGB BaseColor;

public:
  Breath(CRGB _BaseColor)
  {
    CycleCount = 0;
    Time = 0;
    BaseColor = _BaseColor;
    increasing = true;
  }
  ~Breath()
  {
  }
  void Update(int deltaMs)
  {
    Time += deltaMs;
    while (Time > BREATH_CYCLE_TIME)
    {
      Time -= BREATH_CYCLE_TIME;
      CycleCount++;
    }
    while (CycleCount > BREATH_STEPS_PER_CYCLE)
    {
      CycleCount -= BREATH_STEPS_PER_CYCLE;
      increasing = !increasing;
    }
    CRGB RenderColor = BaseColor;
    if (increasing)
    {
      RenderColor.nscale8_video(10 + CycleCount);
    }
    else
    {
      RenderColor.nscale8_video(10 + (BREATH_STEPS_PER_CYCLE - CycleCount));
    }
    for (int i = 0; i < ACTIVE_LEDS; i++)
    {
      leds[i] = RenderColor;
    }
  }
  void ReceiveCommand(Command command)
  {
    //NOOP for now
  }
};

class OnState : public PowerState
{
  IState *inner;

public:
  OnState(LightProgram toRun)
  {
    switch (toRun)
    {
    case LightProgram::Purple_Breath:
      inner = new Breath(CRGB::Purple);
      break;
    }
  }
  OnState(IState *programToResume)
  {
    inner = programToResume;
  }
  ~OnState()
  {
    delete inner;
  }
  bool IsSuspended()
  {
    return false;
  }
  IState *ChildProgram()
  {
    return inner;
  }

  void Update(int deltaMs)
  {
    inner->Update(deltaMs);
  }
  void ReceiveCommand(Command command)
  {
    //NOOP for now
  }
};

#define FIVE_MINUTES 300000
class SuspendState : public PowerState
{

  IState *_suspended;
  IState *_motionWake;
  uint32_t _remainingWakeTime;

public:
  SuspendState(IState *suspended)
  {
    _suspended = suspended;
    CRGB suspendColor = CRGB::Purple;
    suspendColor.nscale8_video(35);
    _motionWake = new Breath(CRGB::Purple);
    _remainingWakeTime = 0;
  }

  ~SuspendState()
  {
    delete _motionWake;
  }

  void Update(int deltaMs)
  {
    _remainingWakeTime = deltaMs > _remainingWakeTime ? 0 : _remainingWakeTime - deltaMs;
    if (digitalRead(MOTION_PIN) == HIGH)
    {
      _remainingWakeTime = FIVE_MINUTES;
    }
    if (_remainingWakeTime > 0)
    {
      _motionWake->Update(deltaMs);
    }
    else
    {
      turnOffLeds();
    }
  }

  bool IsSuspended()
  {
    return true;
  }
  IState *ChildProgram()
  {
    return _suspended;
  }
  void ReceiveCommand(Command command)
  {
    //NOOP for now
  }
};

class LightBoiState : public IState
{
  PowerState *inner;

public:
  LightBoiState()
  {
    //inner = new OnState(LightProgram::Purple_Breath);
    inner = new OnState(LightProgram::Purple_Breath);
  }
  ~LightBoiState()
  {
    delete inner;
  }

  void TogglePower()
  {
    IState *childProgram = inner->ChildProgram();
    if (childProgram != nullptr)
    {
      PowerState *prev = inner;
      if (inner->IsSuspended())
      {
        inner = new OnState(childProgram);
      }
      else
      {
        inner = new SuspendState(childProgram);
      }
      delete prev;
    }
  }
  void Update(int deltaMs)
  {
    if (irrecv.decode(&ir_results))
    {
      Serial.println(ir_results.value, BIN);
      irrecv.resume();
      switch (ir_results.value)
      {
      case REMOTE_PWR:
        turnOffLeds();
        // TogglePower();
        break;
      default:
        turnOffLeds();
        break;
      }
    }
    inner->Update(deltaMs);
  }
  void ReceiveCommand(Command command)
  {
    //Top Level Object, Sends commands but does not receive.
  }
};

IState *state;

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn();
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  state = new LightBoiState();
  pinMode(MOTION_PIN, INPUT);
}

void loop()
{
  if (millis() - time_now > PERIOD_MS)
  {
    time_now = millis();
    state->Update(PERIOD_MS);
    FastLED.show();
  }
}