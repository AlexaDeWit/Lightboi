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
CRGB leds[NUM_LEDS];

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
};

class OnState : public IState
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
  ~OnState()
  {
    delete inner;
  }

  void Update(int deltaMs)
  {
    inner->Update(deltaMs);
  }
};

#define FIVE_MINUTES 300000
class SuspendState : public IState
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
};

class LightBoiState : public IState
{
  IState *inner;

public:
  LightBoiState()
  {
    //inner = new OnState(LightProgram::Purple_Breath);
    inner = new SuspendState(new OnState(LightProgram::Purple_Breath));
  }
  ~LightBoiState()
  {
    delete inner;
  }

  void Update(int deltaMs)
  {
    inner->Update(deltaMs);
  }
};

IState *state;

void setup()
{
  state = new LightBoiState();
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  pinMode(MOTION_PIN, INPUT);
}

void loop()
{
  delay(PERIOD_MS);
  state->Update(PERIOD_MS);
  FastLED.show();
}