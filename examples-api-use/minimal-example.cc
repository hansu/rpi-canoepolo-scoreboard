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

using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool bUpdateDisplay = true;

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
  RGBMatrix::Options defaults;
  defaults.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
  defaults.rows = 32;
  defaults.chain_length = 1;
  defaults.parallel = 1;
  defaults.show_refresh_rate = false;
  defaults.pwm_lsb_nanoseconds = 200;
  defaults.brightness = 50;
  defaults.multiplexing = 6;
  defaults.inverse_colors = false;
  defaults.led_rgb_sequence = "BGR";
  char sScoreA[24], sScoreB[24];
  char sTime[24];

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
  rgb_matrix::Color outline_color(0,0,0);
  int letter_spacing = 0;
  bool with_outline = false;

  /*
   * Load font. This needs to be a filename with a bdf bitmap font.
   */
  rgb_matrix::Font font;
  if (!font.LoadFont("../fonts/6x13B.bdf")) {
    fprintf(stderr, "Couldn't load font '%s'\n", "../fonts/7x13.bdf");
    return 1;
  }
  /*
   * If we want an outline around the font, we create a new font with
   * the original font as a template that is just an outline font.
   */
  rgb_matrix::Font *outline_font = NULL;
  if (with_outline) {
      outline_font = font.CreateOutlineFont();
  }

  DisplayData dispData;
  std::thread inputThread(KeyboardInput, std::ref(dispData));


  while(1){
    dispData.updateTime();

    if(bUpdateDisplay){
      sprintf(sScoreA, "%2d ", dispData.getScoreA());
      sprintf(sScoreB, "%2d", dispData.getScoreB());
      sprintf(sTime, "%02d:%02d", dispData.getMin(), dispData.getSec());
      rgb_matrix::DrawText(canvas, font, 0, -1 + font.baseline(), teamAColor, outline_font ? NULL : &bg_color, sScoreA, letter_spacing);
      rgb_matrix::DrawText(canvas, font, 18, -1 + font.baseline(), teamBColor, outline_font ? NULL : &bg_color, sScoreB, letter_spacing);
      rgb_matrix::DrawText(canvas, font, 0, 10 + font.baseline(), timeColor, outline_font ? NULL : &bg_color, sTime, letter_spacing);
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

void KeyboardInput(DisplayData& dispData)
{
  char nInput;
  static int nResetCnt=0;

  while(1){
    printw("%2d %02d:%02d %2d\r", dispData.getScoreA(), dispData.getMin(), dispData.getSec(), dispData.getScoreB());
    // Check input
    nInput = getch();
    switch(nInput){
      case '1': dispData.incScoreA(); break;
      case '2': dispData.incScoreB(); break;
      case 'r': nResetCnt++;
        if(nResetCnt > 40)
        {
          dispData.resetScore();
          nResetCnt = 0;
        }
        break;
      case '4': dispData.setTime(600); break;
      case '5': dispData.startTimer(); break;
      case '6': dispData.stopTimer(); break;
      case '7': dispData.setTime(10); break;
      case 'q': bExit = true; break;
      //case 'x': std::exit(0);//std::terminate();
    }
    bUpdateDisplay = true;
  }
}

