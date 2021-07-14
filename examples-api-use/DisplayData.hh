/*
 * DisplayData.hh
 *
 *  Created on: 08.05.2021
 *      Author: Hans Unzner
 */

#ifndef _DISPLAYDATA_HH_
#define _DISPLAYDATA_HH_

#include <unistd.h>
#include <time.h>

typedef enum states { idle, running, paused } states_t;
typedef enum colors { white, yellow, red, blue, green, orange, violet } colors_t;
#define NUM_COLORS 5


class DisplayData
{
public:
  DisplayData() : m_nScoreA(0), m_nScoreB(0), m_nPlayTimeSec(600), m_nShotTimeout(60), m_teamAColorIndex(white), m_teamBColorIndex(white),
   m_nStartTime(0), m_nSeconds(0), m_nSecondsLast(0), m_bTimerStarted(false),
  m_state(idle), m_shotclockState(idle), m_bUpdateDisplay(true), m_bUpdateShotclock(true) { }

  int getScoreA()  { return m_nScoreA; }

  int getScoreB()  { return m_nScoreB; }

  void setScoreA(int score)  { m_nScoreA = score; }

  void setScoreB(int score)  { m_nScoreB = score; }

  void incScoreA()  {
    m_nScoreA++;
    resetShotclock();
    m_shotclockState = paused;
  }

  void incScoreB()  {
    m_nScoreB++;
    resetShotclock();
    m_shotclockState = paused;
  }

  void decScoreA()  {
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

  void resetShotclock()
  {
    m_nShotTimeout = 60;
    m_shotclockState = running;
  }

  int getMin()  { return m_nPlayTimeSec/60; }

  int getSec()  { return m_nPlayTimeSec%60; }

  int getTime()  { return m_nPlayTimeSec; }

  int getShotTimeout()  { return m_nShotTimeout; }

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

  void decTime()  { m_nPlayTimeSec--; }

  void modifyTime(int nValue)
  {
    if(m_state != running){
      m_nPlayTimeSec += nValue;

      // only for testing to be able to select 5 seconds
      if(m_nPlayTimeSec == -57)
        m_nPlayTimeSec = 0;
      else if(m_nPlayTimeSec == -60)
        m_nPlayTimeSec = 0;
      else if(m_nPlayTimeSec <= 0)
        m_nPlayTimeSec = 3;
    }
  }

  void startTimer()
  {
    m_nStartTime = time(NULL);  // get current time
    m_nSeconds = 0;
    m_bTimerStarted = true;
    m_shotclockState = running;
  }

  void stopTimer()  { m_bTimerStarted = false; }

  void updateTime()
  {
    if(m_bTimerStarted)
    {
      m_nSecondsLast = m_nSeconds;
      m_nSeconds = (int)difftime(time(NULL), m_nStartTime);
      if(m_nSecondsLast != m_nSeconds)
      {
        m_nPlayTimeSec -= (m_nSeconds - m_nSecondsLast);
        if(m_nPlayTimeSec <= 0)
        {
          m_nPlayTimeSec = 0;
          m_bTimerStarted = false;
          m_state = idle;
        }
        m_bUpdateDisplay = true;

        if(m_shotclockState == running){
          m_nShotTimeout -= (m_nSeconds - m_nSecondsLast);
          if(m_nShotTimeout <= 0){
            m_nShotTimeout = 0;
          }
          m_bUpdateShotclock = true;
        }
      }
    }
  }
  void setState(states_t state)  { m_state = state; }

  states_t getState(void)
  {
    return m_state;
  }

  void start_pause(){
    if(m_state == running){
      stopTimer();
      setState(paused);
      m_shotclockState = paused;
    } else {
      startTimer();
      setState(running);
      m_shotclockState = running;
    }
  }

  colors_t getColorIndexA(void){ return m_teamAColorIndex; }

  colors_t getColorIndexB(void){ return m_teamBColorIndex; }

  void swapTeamColors(void){
    colors_t tmp = m_teamAColorIndex;
    m_teamAColorIndex = m_teamBColorIndex;
    m_teamBColorIndex = tmp;
  }

  void resetColors(void){
    m_teamAColorIndex = white;
    m_teamBColorIndex = white;
  }

  void nextColorIndexA(void){
    if (m_state != running){
      m_teamAColorIndex = (colors_t)((int)m_teamAColorIndex+1);
      if(m_teamAColorIndex >= NUM_COLORS)
        m_teamAColorIndex = (colors_t)0;
    }
  }

  void nextColorIndexB(void){
    if (m_state != running){
      m_teamBColorIndex = (colors_t)((int)m_teamBColorIndex+1);
      if(m_teamBColorIndex >= NUM_COLORS)
        m_teamBColorIndex = (colors_t)0;
    }
  }

  bool NeedRefresh(void){
	  return m_bUpdateDisplay;
  }

  bool NeedShotclockRefresh(void){
	  return m_bUpdateShotclock;
  }

  void SetRefresh(bool val){
	  m_bUpdateDisplay = val;
  }
private:
  int m_nScoreA, m_nScoreB, m_nPlayTimeSec, m_nShotTimeout;
  colors_t m_teamAColorIndex, m_teamBColorIndex;
  time_t m_nStartTime;
  int m_nSeconds, m_nSecondsLast;
  bool m_bTimerStarted;
  states_t m_state, m_shotclockState;
  bool m_bUpdateDisplay, m_bUpdateShotclock;


};



#endif /* _DISPLAYDATA_HH_ */
