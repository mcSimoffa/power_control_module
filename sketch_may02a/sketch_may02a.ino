#include <TimerOne.h>
#include <MsTimer2.h>
#include <Button.h>


//------------------------------------------
//подключение входных выводов
#define buttonON  3
//подключенеи выходных выводов
#define relay0      10
#define relay1      9
#define led0        10
#define lampSemi    11
#define lampFull    12
#define overTempPin  7

#define OVERHEAT    24000 //36.6 град
#define COOLSTATE   29000 //23 град

#define POWER_OFF 0
#define POWER_SEMI 1
#define POWER_ON  2
#define CAPASITOR_CHARGE_TIME_MS 2000   // время зарядки (мс)
#define SIGNAL_LEVEL_TO_OFF 500 //сигнал при котором нет автоотключения 
#define TIME_TO_OFF         600000  //time to off (ms)
//------------------------------------------
Button btn(buttonON);
volatile bool overheat_flag = false;
volatile unsigned long TempM;
volatile unsigned long TempS;
volatile bool light_flag = false;
//------------------------------------------
void setup()
{
  pinMode(relay0, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(lampSemi, OUTPUT);
  pinMode(lampFull, OUTPUT);
  pinMode(led0, OUTPUT);
  pinMode(overTempPin, OUTPUT);
  btn.begin();
  pinMode(A1, INPUT);
  pinMode(A0, INPUT);
  digitalWrite(A1, HIGH);//вкл подтягивающих резисторов
  
  MsTimer2::set(10, Timer10); //таймер 10 мсек
  MsTimer2::start();
  Serial.begin(115200);
  Serial.print("\r\nDevice started !\r\n");
}
//------------------------------------------------
/* it return:
POWER_OFF, POWER_SEMI or POWER_ON
*/
int power_state_machine()
{
  static unsigned long timePoint;
  static unsigned long last_input_inpuls;
  static unsigned int state = POWER_OFF;
  unsigned long line_input;
  bool isButtonPressed = btn.pressed();
  switch(state)
  {
    case POWER_OFF:
      if (isButtonPressed)
      {
        state =  POWER_SEMI;
        Serial.print("\r\nSEMI POWER");
        timePoint = millis(); 
      }    
      break;

    case POWER_SEMI:
      if (millis()-timePoint > CAPASITOR_CHARGE_TIME_MS)
      {
        state = POWER_ON;
        last_input_inpuls = millis();
        Serial.print("\r\nPOWER ON");
      }    
      break;

    case POWER_ON:
      if (isButtonPressed)
      {    
        state =  POWER_OFF;
        Serial.print("\r\nPOWER OFF");
      }
      //line input detect
      line_input = (unsigned long)analogRead(A0); //чтение линейного входа
      //раскомментировать чтобы видеть значения сигнала линейного входа
      //Serial.print("\nLine Input ="); Serial.print(line_input);
      if (line_input > SIGNAL_LEVEL_TO_OFF)
      {
        last_input_inpuls = millis();
        Serial.print("\nPower Off timer restart");
      }

  if (millis() - last_input_inpuls > TIME_TO_OFF)
  {
    Serial.print("\nSLEEP mode inter");
    state =POWER_OFF;
  } 
      break;  
  }     
  return (state);
}

//------------------------------------------
void loop()
{
  TempM = (unsigned long)analogRead(A1) << 7; //чтение термодатчиков
  unsigned long AverM = TempS - (TempS >> 7) + (TempM >> 7); //экспотенциальный фильтр
  TempS = AverM;
// раскомменть чтобы видеть температурные значения
  //Serial.print("\ntemperature ="); Serial.print(TempS);
  if (TempS < OVERHEAT)
    overheat_flag = true;
  if (TempS > COOLSTATE)
    overheat_flag = false;  
    
  int ledlight = power_state_machine(); 
  
  if (ledlight == POWER_ON)
  {
    digitalWrite(relay0, 1);
    digitalWrite(relay1, 1);
    if (overheat_flag)
    {
      digitalWrite(lampFull, light_flag);
      digitalWrite(lampSemi, light_flag);
      digitalWrite(overTempPin, 1);
    }
    else
    {
      digitalWrite(lampFull, 1);
      digitalWrite(lampSemi, 1);
      digitalWrite(overTempPin, 0);
    }
  }
  if (ledlight == POWER_SEMI)
  {
    digitalWrite(lampFull, 0);
    digitalWrite(lampSemi, 1);
    digitalWrite(relay0, 1);
    digitalWrite(relay1, 0);
  }
  if (ledlight == POWER_OFF)
  {
    digitalWrite(lampFull, 0);
    digitalWrite(lampSemi, 0);
    digitalWrite(relay0, 0);
    digitalWrite(relay1, 0);
  } 
}
//------------------------------------------
void Timer10()
{
  #define CNT_LIGHT 30
  #define CNT_DARK  30
  static int light= CNT_LIGHT;
  static int dark = CNT_DARK;
  if (overheat_flag)
  {
    if (light_flag)
    {
      if (!light--)
      {
        light_flag = false;
        light= CNT_LIGHT;
        //digitalWrite(lampFull, 0);
      }
    } 
    else
    {
      if (!dark--)
      {
        light_flag = true;
        dark = CNT_DARK;
        //digitalWrite(lampFull, 1);
      }
    }
  }
}
