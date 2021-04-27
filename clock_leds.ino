/*===========================================================
 * Simple LED patterns, intended for back-lit clocks.
 * 
 * Driven by Tiny85, but with Uno support for debugging.   
 */


#if defined(__AVR_ATmega328P__)
#define PLATFORM_UNO
#elif defined(__AVR_ATtiny85__)
#define PLATFORM_TINY
#else
#error "PLATFORM UNSUPPORTED"
#endif

#ifdef PLATFORM_UNO
#define DEBUG_PRINT(item) Serial.print(item)
#define DEBUG_PRINTLN(item) Serial.println(item)
#else
#define DEBUG_PRINT(item)
#define DEBUG_PRINTLN(item)
#endif


#include <Adafruit_NeoPixel.h>

// Which pin to use for DualRingLED control
#define LED_PIN    4  // ATTINY pin 3 is D4
#define POT_PIN    A1 // ATTINY pin 7 is A1
#define NUMPIXELS  8  

#ifdef PLATFORM_TINY
#define BUTTON_PIN 0  // ATTINY pin 5 is D0
#else
#define BUTTON_PIN 8  // Can't use D0 on Uno...reserved for serial.
#endif
#define DEBOUNCE_MS 50

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB+NEO_KHZ800);

#define COLOR_RED     0xFF0000
#define COLOR_GREEN   0x00FF00
#define COLOR_BLUE    0x0000FF
#define COLOR_MAGENTA 0xFF00FF
#define COLOR_YELLOW  0xFFFF00
#define COLOR_CYAN    0x00FFFF
#define COLOR_BLACK   0
#define COLOR_WHITE   0xFFFFFF

typedef enum 
{
  MODE_SOLID_GREEN,
  MODE_GREEN_BREATHING,
  MODE_GREEN_ROTATE,
  NUM_MODES
} mode_type;

void init_solid_green(void);
void process_solid_green(void);
void init_green_breathing(void);
void process_green_breathing(void);
void init_green_rotate(void);
void process_green_rotate(void);

typedef void (*init_func_type)(void);
typedef void (*process_func_type)(void);

init_func_type init_table[] = 
{
  init_solid_green,
  init_green_breathing,
  init_green_rotate
};

process_func_type processing_table[] =
{
  process_solid_green,
  process_green_breathing,
  process_green_rotate
};

int current_mode=0;

uint32_t update_rate_ms = 1;
uint32_t last_update_ms=0;

int brightness=1023;  // 1023 = full brightness.  0 = off.

// this function takes a 24-bit CRGB color and returns a color 
// scaled down by the global brightness factor.
uint32_t set_brightness(uint32_t color)
{
  uint32_t scaled_brightness=0;
  uint32_t  r_byte;
  uint32_t  g_byte;
  uint32_t  b_byte;
  
  //brightness is going to map to a percentage for us to apply to each 8 bits of the RGB value.
  // 1023 will map to full (divide by 1)
  // 0 will map to off (set to 0)
  // for anything in between, we'll use a fraction...so 512 will be half brightness.

  // start with the B byte...easiest
  b_byte = color & 0x0000FF;
  b_byte = b_byte * brightness / 1023;

  g_byte = color & 0x00FF00;
  g_byte = g_byte >> 8;
  g_byte = g_byte * brightness / 1023;

  r_byte = color & 0xFF0000;
  r_byte = r_byte >> 16;
  r_byte = r_byte * brightness / 1023;

  // Now that we have the individual bytes, build back the composite color.
  scaled_brightness = r_byte << 16;
  scaled_brightness = scaled_brightness | (g_byte << 8);
  scaled_brightness = scaled_brightness | b_byte;
  
  return (scaled_brightness);
}

void init_solid_green(void)
{
  int i;

  update_rate_ms = 10;
  
  // set all pixels to green.
  for (i=0; i<NUMPIXELS; i++)
  {
    pixels.setPixelColor(i,COLOR_GREEN);
  }
  pixels.show();
}

void process_solid_green(void)
{
  int i;
  uint32_t adjusted_green;

  brightness = analogRead(POT_PIN);
  
  adjusted_green = set_brightness(COLOR_GREEN);
  
  // set all pixels to a green scaled by our brightness.
  for (i=0; i<NUMPIXELS; i++)
  {
    pixels.setPixelColor(i,adjusted_green);
  }
  pixels.show();
  
}

void init_green_breathing( void )
{
  
}


void process_green_breathing(void)
{
  static int last_top_brightness_setting=0;
  int        current_top_brightness_setting;
  static int dir=-1;  // -1 means down, +1 means up.
  int i;
  int step_size;

  // first, check to see if our brighness has changed. 
  // if it has, we want to restart at the top of our range.
  current_top_brightness_setting = analogRead(POT_PIN);
  if ((current_top_brightness_setting >= last_top_brightness_setting + 10) || 
      (current_top_brightness_setting <= last_top_brightness_setting - 10))
  {
    last_top_brightness_setting = current_top_brightness_setting;
    brightness = current_top_brightness_setting;
    dir = -1;

    // DEBUG_PRINT("New top brightness setting: ");
    // DEBUG_PRINTLN(brightness);
  }
  else
  {
    // DEBUG_PRINT("last brightness: ");
    // DEBUG_PRINTLN(brightness);
 
    if      (brightness < 128) step_size = 1;
    else if (brightness < 256)  step_size = 2;
    else if (brightness < 512)  step_size = 4;
    else                        step_size = 8;
    
    //Otherwise, we're either walking our current brightness down or up.
    brightness += dir*step_size;

    // if we've hit either the top or bottom of our range, reverse direction
    if ((brightness <= 0) || (brightness >= last_top_brightness_setting))
    {
      // DEBUG_PRINTLN("Reversing direction");
      
      dir = dir * -1;
    }
  }

  // finally, show all the pixels at our current brightness.
  for (i=0; i<NUMPIXELS; i++)
  {
    pixels.setPixelColor(i,set_brightness(COLOR_GREEN));
  }
  pixels.show();
}

// Fun UI thought:  we set brightness based on the dial before we enter this mode...
// then we use the dial to adjust speed!!

void init_green_rotate( void )
{
  int i;
  uint32_t temp_color;

  DEBUG_PRINTLN("green rotate init");
  
  // We're going to make a "streak" that's half way around...and have to deal with the fact
  // that brightness seems to be logarithmic
  brightness = 1023;
  for (i = 0; i < NUMPIXELS/2; i++)
  {
    temp_color = set_brightness(COLOR_GREEN);
    pixels.setPixelColor(i, temp_color);

    if (brightness > 500) brightness = 250;
    else (brightness = brightness / 2);  
  }
  
  for(;i<NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, COLOR_BLACK);
  }
  

  pixels.show();

  for (i = 0; i<NUMPIXELS; i++)
  {
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(pixels.getPixelColor(i), HEX);
  }

  

}

void process_green_rotate( void )
{
  uint32_t roll_over_color;
  uint32_t temp_color;
  int i;

  //DEBUG_PRINTLN("Process rotate");
  
  // in here, we're going to use the pot to determine rotate speed.  This means
  // we'll inherit brightness from the previous modes.
  update_rate_ms = analogRead(POT_PIN);
  
  roll_over_color = pixels.getPixelColor(0);
  for (i = 0; i < NUMPIXELS-1; i++)
  {
    temp_color = pixels.getPixelColor(i+1);
    
    pixels.setPixelColor(i, temp_color);
  }

  
  pixels.setPixelColor(NUMPIXELS - 1, roll_over_color);

  pixels.show();

}

void setup()
{
    int i;
    
    #ifdef PLATFORM_UNO
    Serial.begin(9600);
    #endif
    
    pixels.begin();
    // Power on self-test...see if all the pixels are working.
    for (i=0; i<NUMPIXELS; i++)
    {
      pixels.setPixelColor(i,COLOR_GREEN);
      delay(50);
      pixels.show();
    }

    #ifdef PLATFORM_UNO
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    #else
    pinMode(BUTTON_PIN, INPUT);
    #endif
    
    init_solid_green();

}

void loop()
{
  uint32_t        current_ms;
  static uint32_t press_ms=0;   // use this for debouncing.
  static int      last_button_state = HIGH;
  int             current_button_state;
  
  // check for button presses to switch modes.
  current_button_state = digitalRead(BUTTON_PIN);
  if ((current_button_state == HIGH) && (last_button_state == LOW))
  {
    // check timestamp on releases
    current_ms = millis();
    if (current_ms > press_ms + DEBOUNCE_MS)
    {
      // this was a legit release.  Tweak state
      DEBUG_PRINT("PRESS:  ");
      DEBUG_PRINT(current_mode);
      current_mode = (current_mode + 1) % 3;
      init_table[current_mode]();
    }
  }
  else if ((current_button_state == LOW) && (last_button_state == HIGH))
  {
    // Start of a press.  Mark the timestamp.
    press_ms = millis();
  }
  last_button_state = current_button_state;

  // call appropriate processing function
  current_ms = millis();
  if (current_ms > last_update_ms + update_rate_ms)
  {
    last_update_ms = current_ms;
    processing_table[current_mode]();
  }
}
