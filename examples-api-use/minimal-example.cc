// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the library.
// For more examples, look at demo-main.cc
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "graphics.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <thread>
#include "mytimer.h"
#include <time.h>
#include <iostream>
#include <string>
 
using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool bUpdateDisplay = true;
char sScoreA[24], sScoreB[24], sTime[24];

class DisplayData
{
public:
  DisplayData() : m_nScoreA(0), m_nScoreB(0), m_nPlayTimeSec(600), m_bTimerStarted(false)
  {
  }

  int getScoreA()
  {
    return m_nScoreA;
  }

  int getScoreB()
  {
    return m_nScoreB;
  }

  void incScoreA()
  {
    m_nScoreA++;
  }

  void incScoreB()
  {
    m_nScoreB++;
  }

  void decScoreA()
  {
    if(m_nScoreA > 0)
      m_nScoreA--;
  }

  void decScoreB()
  {
    if(m_nScoreB > 0)
      m_nScoreB--;
  }

  void resetScore()
  {
    m_nScoreA = 0;
    m_nScoreB = 0;
  }

  int getMin()
  {
    return m_nPlayTimeSec/60;
  }

  int getSec()
  {
    return m_nPlayTimeSec%60;
  }

  void setTime(int nSec)
  {
    if(!m_bTimerStarted)
      m_nPlayTimeSec = nSec;
  }

  void setTime(int nMin, int nSec)
  {
    if(!m_bTimerStarted)
      m_nPlayTimeSec = nMin*60 + nSec;
  }

  void decTime()
  {
    m_nPlayTimeSec--;
  }

  void startTimer()
  {
    m_nStartTime = time(NULL);  // get current time
    m_nSeconds = 0;
    m_bTimerStarted = true;
  }

  void stopTimer()
  {
    m_bTimerStarted = false;
  }

  void updateTime()
  {
    if(m_bTimerStarted)
    {
      m_nSecondsLast = m_nSeconds;
      m_nSeconds = (int)difftime(time(NULL), m_nStartTime);
      if(m_nSecondsLast != m_nSeconds)
      {
        m_nPlayTimeSec -= (m_nSeconds - m_nSecondsLast);
        if(m_nPlayTimeSec < 0)
        {
          m_nPlayTimeSec = 0;
          m_bTimerStarted = false;
        }
        bUpdateDisplay = true;
      }
    }
  }
private:
  int m_nScoreA, m_nScoreB, m_nPlayTimeSec;
  time_t m_nStartTime;
  int m_nSeconds, m_nSecondsLast;
  bool m_bTimerStarted;


};

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

void KeyboardInput(DisplayData& dispData);
volatile bool bExit = false;


int main(int argc, char *argv[]) {

  rgb_matrix::RuntimeOptions runtime;  
  runtime.gpio_slowdown = 4;
  
  RGBMatrix::Options defaults;
  defaults.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
  defaults.rows = 16;
  defaults.cols = 32;
  defaults.chain_length = 5;
  defaults.parallel = 2;
  //defaults.pixel_mapper_config = "V-mapper:Z;Rotate:90";
  defaults.show_refresh_rate = false;
  //defaults.pwm_lsb_nanoseconds = 200;
  defaults.brightness = 50;
  defaults.multiplexing = 3;
  defaults.inverse_colors = false;
  defaults.led_rgb_sequence = "RGB";

  // initializing curses lib
  initscr();
  noecho();
  timeout(-1); // set to blocking mode - otherwise time out value

  Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &defaults);
  if (canvas == NULL)
    return 1;

  // It is always good to set up a signal handler to cleanly exit when we
  // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
  // for that.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);


  // ab hier
  rgb_matrix::Color timeColor(255, 0, 0);
  rgb_matrix::Color teamAColor(255, 229, 0);
  rgb_matrix::Color teamBColor(0, 200, 255);

  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color outline_color(255,255,255);
  int letter_spacing = 0;

  /*
   * Load bdf bitmap fonts.
   */
  rgb_matrix::Font font_std, font_narr;
  
  if (!font_std.LoadFont("../fonts2/LiberationSansNarrow_bb32.bdf")) {
    fprintf(stderr, "Couldn't load std font '%s'\n", "../fonts2/LiberationSansNarrow_bb32.bdf");
    return 1;
  }
  if (!font_narr.LoadFont("../fonts2/antonio_b32.bdf")) {
    fprintf(stderr, "Couldn't load narrow font '%s'\n", "../fonts2/antonio_b32.bdf");
    return 1;
  }
  
  
  DisplayData dispData;
  std::thread inputThread(KeyboardInput, std::ref(dispData));


  while(1){
    dispData.updateTime();

    if(bUpdateDisplay){
      sprintf(sScoreA, "%2d ", dispData.getScoreA());
      sprintf(sScoreB, "%2d", dispData.getScoreB());
      sprintf(sTime, "%02d:%02d", dispData.getMin(), dispData.getSec());
      canvas->Clear();
      rgb_matrix::DrawText(canvas, font_narr, 0, 32, teamAColor, &bg_color, sScoreA, letter_spacing);
      rgb_matrix::DrawText(canvas, font_std, 36, 32, timeColor,  &bg_color, sTime,   letter_spacing);
      rgb_matrix::DrawText(canvas, font_narr, 128, 32, teamBColor, &bg_color, sScoreB, letter_spacing); // to be adjusted

//      Für Stromaufnahme-Test      
//      canvas->Clear();
//      for(int i=0; i<16; i++){
//       rgb_matrix::DrawLine(canvas, 0, i, 768, i, outline_color);
//      }
      
      bUpdateDisplay = false;
    }
    if(bExit)
    {
      break;
    }
    usleep(200 * 1000);
  }

  endwin();
  canvas->Clear();
  delete canvas;
  return 0;
}

/*
 * Search for str2 in str1
 * Return true if str2 found at the end of str1 
 */
bool rfind_str(std::string &str1, std::string str2){
  return (str1.substr(str1.size()-str2.size(), str2.size()) == str2);
}

void KeyboardInput(DisplayData& dispData)
{
  char nInput;
  static int nResetCnt=0, nExitCnt=0;
  static std::string sBuf = "    ";


  const std::string sIncScoreA =    {"\e[A"};  // up
  const std::string sDecScoreA =    {"\e[B"};  // down  
  const std::string sIncScoreB =    {"\e[5~"}; // pg up
  const std::string sDecScoreB =    {"\e[6~"}; // pg down
 

  while(1){
    // Check input
    nInput = getch();
    switch(nInput){
      case '8': dispData.incScoreA(); break;
      case '2': dispData.decScoreA(); break;            
      case '9': dispData.incScoreB(); break;
      case '3': dispData.decScoreB(); break;
      case 'r': 
      case '*':
      case 127: // backspace
        nResetCnt++;
        if(nResetCnt > 40) // 2 seconds
        {
          dispData.resetScore();
          dispData.stopTimer();
          dispData.setTime(600);
          nResetCnt = 0;
        }      
        break;       
      case '*': dispData.setTime(600); break;
      case 10: // return
      case '+': 
          dispData.startTimer(); break;      
      case '-': dispData.stopTimer(); break;
      case 'q': bExit = true; break;
      //case 'x': std::exit(0);//std::terminate();
      case '/':
        nExitCnt++;
        if(nExitCnt > 150)
        {
          bExit = true;
        }  
        break;
      default: 
        sBuf.push_back(nInput);  
        if(sBuf.size() > 5)
          sBuf.erase(0, 1);
          

         // alternative input when num lock is activated 
        if(rfind_str(sBuf, sIncScoreA))       dispData.incScoreA();
        else if(rfind_str(sBuf, sDecScoreA))  dispData.decScoreA();    
        else if(rfind_str(sBuf, sIncScoreB))  dispData.incScoreB();
        else if(rfind_str(sBuf, sDecScoreB))  dispData.decScoreB();

        break;   
    }
   // printw("%2d %02d:%02d %2d\r", dispData.getScoreA(), dispData.getMin(), dispData.getSec(), dispData.getScoreB());          
    bUpdateDisplay = true;
  }
}

