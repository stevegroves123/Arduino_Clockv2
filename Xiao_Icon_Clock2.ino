
#include <Arduino.h>
#include <U8g2lib.h>
#include <RTCZero.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

unsigned long previousMillis = 0;
const long interval = 250;

uint8_t h = 0;
uint8_t m = 0;
uint8_t s = 0;
uint8_t dy = 1;
uint8_t mth = 1;
uint8_t yr = 20;
uint8_t event;
uint8_t alarmh = 0;
uint8_t alarmm = 0;

bool exitHours = false;
bool exitMins = false;
bool exitDay = false;
bool exitMonth = false;
bool exitYear = false;
bool alarmActive = false;
bool flashDisplay = false;
bool digitalActive = false;

/* Create an rtc object */
RTCZero rtc;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// START OF ANALOG CLOCK
const int MIDDLE_Y = u8g2.getDisplayHeight() / 2;
const int MIDDLE_X = u8g2.getDisplayWidth() / 2;
/* Constants for the clock face */
int CLOCK_RADIUS;
int POS_12_X, POS_12_Y;
int POS_3_X, POS_3_Y;
int POS_6_X, POS_6_Y;
int POS_9_X, POS_9_Y;
int S_LENGTH;
int M_LENGTH;
int H_LENGTH;

unsigned long lastDraw = 0;

void initClockVariables()
{
  // Calculate constants for clock face component positions:

  CLOCK_RADIUS = min(MIDDLE_X, MIDDLE_Y) - 1;

  POS_12_X = MIDDLE_X - 6;
  POS_12_Y = MIDDLE_Y - CLOCK_RADIUS + 12;
  POS_3_X  = MIDDLE_X + CLOCK_RADIUS - 10;
  POS_3_Y  = MIDDLE_Y + 5;
  POS_6_X  = MIDDLE_X - 2;
  POS_6_Y  = MIDDLE_Y + CLOCK_RADIUS - 2;
  POS_9_X  = MIDDLE_X - CLOCK_RADIUS + 4;
  POS_9_Y  = MIDDLE_Y + 5;

  // Calculate clock arm lengths
  S_LENGTH = CLOCK_RADIUS - 2;
  M_LENGTH = S_LENGTH * 0.75;
  H_LENGTH = S_LENGTH * 0.5;
}

void drawArms(int h, int m, int s)
{
  double midHours;  // this will be used to slightly adjust the hour hand
  static int hx, hy, mx, my, sx, sy;

  // Adjust time to shift display 90 degrees ccw
  // this will turn the clock the same direction as text:

  h -= 3;
  m -= 15;
  s -= 15;
  if (h <= 0)
    h += 12;
  if (m < 0)
    m += 60;
  if (s < 0)
    s += 60;
  // Calculate and draw new lines:
  s = map(s, 0, 60, 0, 360);  // map the 0-60, to "360 degrees"
  sx = S_LENGTH * cos(PI * ((float)s) / 180);  // woo trig!
  sy = S_LENGTH * sin(PI * ((float)s) / 180);  // woo trig!
  // draw the second hand:
  u8g2.drawLine(MIDDLE_X, MIDDLE_Y, MIDDLE_X + sx, MIDDLE_Y + sy);

  m = map(m, 0, 60, 0, 360);  // map the 0-60, to "360 degrees"
  mx = M_LENGTH * cos(PI * ((float)m) / 180);  // woo trig!
  my = M_LENGTH * sin(PI * ((float)m) / 180);  // woo trig!
  // draw the minute hand
  u8g2.drawLine(MIDDLE_X, MIDDLE_Y, MIDDLE_X + mx, MIDDLE_Y + my);

  midHours = int(rtc.getMinutes()) / 12; // midHours is used to set the hours hand to middling levels between whole hours
  h *= 5;  // Get hours and midhours to the same scale
  h += midHours;  // add hours and midhours
  h = map(h, 0, 60, 0, 360);  // map the 0-60, to "360 degrees"
  hx = H_LENGTH * cos(PI * ((float)h) / 180);  // woo trig!
  hy = H_LENGTH * sin(PI * ((float)h) / 180);  // woo trig!
  // draw the hour hand:
  u8g2.drawLine(MIDDLE_X, MIDDLE_Y, MIDDLE_X + hx, MIDDLE_Y + hy);
}

// Draw an analog clock face
void drawFace()
{
  // Draw the clock border
  u8g2.drawCircle(MIDDLE_X, MIDDLE_Y, CLOCK_RADIUS, U8G2_DRAW_ALL);
  // Draw the clock numbers
  u8g2.setCursor(POS_12_X, POS_12_Y); // points cursor to x=27 y=0
  u8g2.print("12");
  u8g2.setCursor(POS_6_X, POS_6_Y);
  u8g2.print("6");
  u8g2.setCursor(POS_9_X, POS_9_Y);
  u8g2.print("9");
  u8g2.setCursor(POS_3_X, POS_3_Y);
  u8g2.print("3");
}

void setup(void) {
  u8g2.begin(/*Select=*/ 0, /*Right/Next=*/ 1, /*Left/Prev=*/ 2, /*Up=*/ 4, /*Down=*/ 3, /*Home/Cancel=*/ A6);
  rtc.begin();
  // Set the time
  updatedTimeHMS();
  rtc.setSeconds(int(s));
  // Display the time
  displayTime();

  u8g2.setFont(u8g2_font_6x12_tr);
}

void displayTime()
{
  digitalActive = false;
  u8g2.clearBuffer();
  initClockVariables();
  drawFace();
  drawArms(int(rtc.getHours()), int(rtc.getMinutes()), int(rtc.getSeconds()));
  u8g2.sendBuffer();
}

void updatedTimeHMS() {
  do {
    u8g2.setFont(u8g2_font_7x13_tf);
    u8g2.setFontRefHeightAll();
    if (alarmActive == false) {
      u8g2.userInterfaceInputValue("Update Time", "Hours = ", &h, 0, 23, 2, "");
    } else {
      u8g2.userInterfaceInputValue("Alarm Update", "Hours = ", &h, 0, 23, 2, "");
      alarmh = h;
    }
    if ( event == U8X8_MSG_GPIO_MENU_SELECT )
    {
      exitHours = true;
    }
  } while (exitHours = false);

  do {
    u8g2.setFont(u8g2_font_7x13_tf);
    u8g2.setFontRefHeightAll();
    if (alarmActive == false) {
      u8g2.userInterfaceInputValue("Update Time", "Minutes = ", &m, 0, 59, 2, "");
    } else {
      u8g2.userInterfaceInputValue("Alarm Update", "Minutes = ", &m, 0, 59, 2, "");
      alarmm = m;
    }
    if ( event == U8X8_MSG_GPIO_MENU_SELECT )
    {
      exitMins = true;
    }
  } while (exitMins = false);

  do {
    if (alarmActive == true)
    {
      break; // Don't set the date if changing the alarm time
    }
    u8g2.setFont(u8g2_font_7x13_tf);
    u8g2.setFontRefHeightAll();
    u8g2.userInterfaceInputValue("Update Clock", "Day = ", &dy, 1, 31, 2, "");
    if ( event == U8X8_MSG_GPIO_MENU_SELECT )
    {
      exitDay = true;
    }
  } while (exitDay = false );

  do {
    if (alarmActive == true)
    {
      break;  // Don't set the date if changing the alarm time
    }
    u8g2.setFont(u8g2_font_7x13_tf);
    u8g2.setFontRefHeightAll();
    u8g2.userInterfaceInputValue("Update Clock", "Month = ", &mth, 1, 31, 2, "");
    if ( event == U8X8_MSG_GPIO_MENU_SELECT )
    {
      exitMonth = true;
    }
  } while (exitMonth = false);

  do {
    if (alarmActive == true)
    {
      break;  // Don't set the date if changing the alarm time
    }
    u8g2.setFont(u8g2_font_7x13_tf);
    u8g2.setFontRefHeightAll();
    u8g2.userInterfaceInputValue("Update Clock", "Year = ", &yr, 20, 99, 2, "");
    if ( event == U8X8_MSG_GPIO_MENU_SELECT )
    {
      exitYear = true;
    }
  } while (exitYear = false);

  if (alarmActive == false ) { // if updating time then set the hours and minutes
    rtc.setHours(int(h));
    rtc.setMinutes(int(m));
    rtc.setDay(int(dy));
    rtc.setMonth(int(mth));
    rtc.setYear(int(yr));
  }
}

void displayDigital() {
  u8g2.setFont(u8g2_font_inb19_mf);
  u8g2.clearBuffer();
  u8g2.setCursor(0, 35);
  // time
  addZero(rtc.getHours());
  u8g2.print(":");
  addZero(rtc.getMinutes());
  u8g2.print(":");
  addZero(rtc.getSeconds());
  // date
  u8g2.setFont(u8g2_font_crox3c_mf);
  u8g2.setCursor(20, 54);
  addZero(rtc.getDay());
  u8g2.print("/");
  addZero(rtc.getMonth());
  u8g2.print("/");
  addZero(rtc.getYear());
  u8g2.sendBuffer();
}

void addZero(int addZero) {
  if (addZero < 10) {
    u8g2.print("0");
  }
  u8g2.print(addZero);
}

void alarmActivated()
{
  flashDisplay = true;
}

// Alarm section
void flashAlarm() {
  if (flashDisplay == true ) {
    do
    {
      unsigned long currentMillis = millis();
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      if (currentMillis - previousMillis  >= interval) {
        previousMillis = currentMillis;
        u8g2.drawBox(0, 0, 128, 64);
        u8g2.sendBuffer();
      }
    } while ( u8g2.getMenuEvent() == 0 ); // leave in alarm mode until enter key pressed
    flashDisplay = false;
  }
}


// ------------------------- Icon settings ---------------------------------
struct menu_entry_type
{
  const uint8_t *font;
  uint16_t icon;
  const char *name;
};

struct menu_state
{
  int16_t menu_start;		/* in pixel */
  int16_t frame_position;		/* in pixel */
  uint8_t position;			/* position, array index */
};

#define ICON_WIDTH 32
#define ICON_HEIGHT 32
#define ICON_GAP 4
#define ICON_BGAP 16
#define ICON_Y 32+ ICON_GAP

struct menu_entry_type menu_entry_list[] =
{
  { u8g2_font_open_iconic_embedded_4x_t, 68, "Clock"},
  { u8g2_font_open_iconic_embedded_4x_t, 65, "Alarm"},
  { u8g2_font_open_iconic_embedded_4x_t, 77, "Light"},
  { u8g2_font_open_iconic_embedded_4x_t, 72, "Settings"},
  { NULL, 0, NULL }
};

void draw(struct menu_state *state)
{
  int16_t x;
  uint8_t i;
  x = state->menu_start;
  i = 0;
  while ( menu_entry_list[i].icon > 0 )
  {
    if ( x >= -ICON_WIDTH && x < u8g2.getDisplayWidth() )
    {
      u8g2.setFont(menu_entry_list[i].font);
      u8g2.drawGlyph(x, ICON_Y, menu_entry_list[i].icon );
    }
    i++;
    x += ICON_WIDTH + ICON_GAP;
  }
  u8g2.drawFrame(state->frame_position - 1, ICON_Y - ICON_HEIGHT - 1, ICON_WIDTH + 2, ICON_WIDTH + 2);
  u8g2.drawFrame(state->frame_position - 2, ICON_Y - ICON_HEIGHT - 2, ICON_WIDTH + 4, ICON_WIDTH + 4);
  u8g2.drawFrame(state->frame_position - 3, ICON_Y - ICON_HEIGHT - 3, ICON_WIDTH + 6, ICON_WIDTH + 6);
}

// when you press the right button
void to_right(struct menu_state *state)
{
  if ( menu_entry_list[state->position + 1].font != NULL )
  {
    if ( (int16_t)state->frame_position + 2 * (int16_t)ICON_WIDTH + (int16_t)ICON_BGAP < (int16_t)u8g2.getDisplayWidth() )
    {
      state->position++;
      state->frame_position += ICON_WIDTH + (int16_t)ICON_GAP;
    }
    else
    {
      state->position++;
      state->frame_position = (int16_t)u8g2.getDisplayWidth() - (int16_t)ICON_WIDTH - (int16_t)ICON_BGAP;
      state->menu_start = state->frame_position - state->position * ((int16_t)ICON_WIDTH + (int16_t)ICON_GAP);
    }
  }
}

// when you press the left button
void to_left(struct menu_state *state)
{
  if ( state->position > 0 )
  {
    if ( (int16_t)state->frame_position >= (int16_t)ICON_BGAP + (int16_t)ICON_WIDTH + (int16_t)ICON_GAP )
    {
      state->position--;
      state->frame_position -= ICON_WIDTH + (int16_t)ICON_GAP;
    }
    else
    {
      state->position--;
      state->frame_position = ICON_BGAP;
      state->menu_start = state->frame_position - state->position * ((int16_t)ICON_WIDTH + (int16_t)ICON_GAP);

    }
  }
}

uint8_t towards_int16(int16_t *current, int16_t dest)
{
  if ( *current < dest )
  {
    (*current)++;
    return 1;
  }
  else if ( *current > dest )
  {
    (*current)--;
    return 1;
  }
  return 0;
}

uint8_t towards(struct menu_state *current, struct menu_state *destination)
{
  uint8_t r = 0;
  r |= towards_int16( &(current->frame_position), destination->frame_position);
  r |= towards_int16( &(current->frame_position), destination->frame_position);
  r |= towards_int16( &(current->menu_start), destination->menu_start);
  r |= towards_int16( &(current->menu_start), destination->menu_start);
  return r;
}

struct menu_state current_state = { ICON_BGAP, ICON_BGAP, 0 };
struct menu_state destination_state = { ICON_BGAP, ICON_BGAP, 0 };


void loop(void) {
  int8_t event;
  flashAlarm();
  do
  {
    
    u8g2.clearBuffer();
    draw(&current_state);
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.setCursor((u8g2.getDisplayWidth() - u8g2.getStrWidth(menu_entry_list[destination_state.position].name)) / 2, u8g2.getDisplayHeight() - 5);
    u8g2.print(menu_entry_list[destination_state.position].name);
    u8g2.sendBuffer();
    delay(1);
    event = u8g2.getMenuEvent();
    if ( event == U8X8_MSG_GPIO_MENU_NEXT )
      to_right(&destination_state);
    if ( event == U8X8_MSG_GPIO_MENU_PREV )
      to_left(&destination_state);
    if ( event == U8X8_MSG_GPIO_MENU_SELECT )
    {
      if (menu_entry_list[destination_state.position].name == "Clock")
      {
        u8g2.setFont(u8g2_font_helvB10_tr);
        do {
          if (digitalActive == false) {
            displayTime();
            flashAlarm();
          } else {
            displayDigital();
            flashAlarm();
          }
        } while ( u8g2.getMenuEvent() == 0 );
      }

      if (menu_entry_list[destination_state.position].name == "Alarm")
      {
        alarmActive = true;
        updatedTimeHMS();
        rtc.setAlarmTime(alarmh, alarmm, s);
        rtc.enableAlarm(rtc.MATCH_HHMMSS);
        rtc.attachInterrupt(alarmActivated);
        
        u8g2.clearBuffer();
        u8g2.clearDisplay();
        u8g2.drawStr(5, 22, "-----------------");
        u8g2.drawStr(5, 32, "| Alarm set for |");
        u8g2.drawStr(5, 44, "|");
        u8g2.setCursor(50, 44);
        addZero(alarmh);
        u8g2.print(":");
        addZero(alarmm);
        u8g2.drawStr(117, 44, "|");
        u8g2.drawStr(5, 52, "-----------------");
        u8g2.sendBuffer();
        delay(2000);
      }

      if (menu_entry_list[destination_state.position].name == "Settings")
      {
        u8g2.setFont(u8g2_font_helvB10_tr); // change font as too big
        int settingsKey = u8g2.userInterfaceSelectionList("Settings", 1, "Back\nDigital\nAnalog\nTest");
        u8g2.setFontRefHeightAll();
        
        if ( settingsKey == 2 ) {  // digital option from menu
          u8g2.clearBuffer();
          u8g2.clearDisplay();
          u8g2.drawStr(10, 22, "Digital display");
          u8g2.drawStr(10, 38, "    ACTIVE     ");
          u8g2.sendBuffer();
          delay(2000);
          digitalActive = true;
        }

        if ( settingsKey == 3 ) {  // analog option from menu
          u8g2.clearBuffer();
          u8g2.clearDisplay();
          u8g2.drawStr(10, 22, "Analog display");
          u8g2.drawStr(10, 38, "    ACTIVE    ");
          u8g2.sendBuffer();
          delay(2000);
          digitalActive = false;
        }

        if ( settingsKey == 4 ) {  // test optin from menu
          u8g2.clearBuffer();
          u8g2.clearDisplay();
          char s[2] = " ";
          uint8_t x, y;
          for ( y = 0; y < 6; y++ ) {
            for ( x = 0; x < 16; x++ ) {
              s[0] = y * 16 + x + 32;
              u8g2.drawStr(x * 7, y * 10 + 10, s);
              u8g2.sendBuffer();
            }
          }
          delay(2000);
        }
      }

      if (menu_entry_list[destination_state.position].name == "Light")
      {
        u8g2.clearBuffer();
        u8g2.clearDisplay();
        u8g2.setDrawColor(1);
        do {
          u8g2.drawBox(0, 0, 130, 65);
          u8g2.sendBuffer();
        } while ( u8g2.getMenuEvent() == 0 );
      }
    }
  } while ( towards(&current_state, &destination_state) || flashDisplay != true);
}
