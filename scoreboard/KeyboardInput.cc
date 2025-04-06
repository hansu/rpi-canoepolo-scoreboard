#include "led-matrix.h"
#include "graphics.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <iostream>
#include <string>
#include "DisplayData.hh"
#include "socket.hh"
#include <sstream>

extern volatile bool bExit;
extern volatile bool use_ncurses_gl;
// #define DEBUG_KEYS

#ifndef CROSS_COMPILING
#define USE_NCURSES
#endif

#ifdef USE_NCURSES
#include <curses.h>
#endif

/*
 * Search for str2 in str1
 * Return true if str2 found at the end of str1
 */
bool rfind_str(std::string &str1, std::string str2){
  if(str1.size()>=str2.size())
    return (str1.substr(str1.size()-str2.size(), str2.size()) == str2);
  else
    return false;
}

/*
 * Find string in vector<string>
 * If first char if str1 is not ESC then it compares to the second element of vec
 */
bool find_seq(std::string &str1, std::vector<std::string> vec){
  if(rfind_str(str1, vec[0]))
    return true;
  else if(rfind_str(str1, vec[1]) && str1[str1.length()-3] != '\e')
    return true;
  return false;
}

void KeyboardInput(DisplayData& dispData)
{
  char nInput;
  static int nResetCnt=0, nShutdownCnt=0, nDispSwitchCnt=0;
  static std::string sBuf = "    ";

  const std::vector<std::string> colorChangeA = {"\e[A", "8"};  // up
  const std::vector<std::string> decScoreA =    {"\e[G", "5"};  // keypad 5, 5
  const std::vector<std::string> incScoreA =    {"\e[B", "2"};  // down

  const std::vector<std::string> colorChangeB = {"\e[5~", "9"}; // pg up
  const std::vector<std::string> decScoreB =    {"\e[C", "6"};  // right, 6
  const std::vector<std::string> incScoreB =    {"\e[6~", "3"}; // pg down

  const std::vector<std::string> start_pause = {"\e[3~", ","}; // comma, del
  const std::vector<std::string> resetShotclock = {"\e[2~", "0"}; // 0, ins

  while(1){
    // Check input
    if (use_ncurses_gl){
      #ifdef USE_NCURSES
      nInput = getch();
      #endif
    } else{
      nInput = getchar();
    }
#ifdef DEBUG_KEYS
    std::cout << nInput << "-------" << std::endl;
#endif
    switch(nInput){
      case 'r':
        nShutdownCnt++;
        if(nShutdownCnt > 150)
        {
          //system("echo raspberry | sudo -S poweroff");
          //system("sudo shutdown now");
        }
        break;
      case 127: // backspace      (delete = 8 /'\b')?
        nDispSwitchCnt++;
        if(nDispSwitchCnt > 40) // 2 seconds
        {
        }
        break;
      // Swap fields/teams
      case '*':
        dispData.swapTeams();
        break;
      // Start/pause
#ifndef CROSS_COMPILING
      case 10: // return
        dispData.start_pause();
        break;
#endif
      case '+':
        dispData.modifyTime(60);
        break;
      case '-':
        dispData.modifyTime(-60);
        break;
      case 'q':
        bExit = true;
        break;
      // Stop
      case '/':
        dispData.stopTimer();
        dispData.setState(idle);
        nResetCnt++;
        if(nResetCnt > 40) // 2 seconds
        {
          dispData.resetScore();
          dispData.stopTimer();
          dispData.setTime(600);
          dispData.resetColors();
          dispData.resetShotclock();
          nResetCnt = 0;
        }
        break;

      default:
        sBuf.push_back(nInput);
        if(sBuf.size() > 4)
          sBuf.erase(0, 1);

        dispData.SetRefresh(true);
        // Keys with two functions
        if(find_seq(sBuf, incScoreA))       dispData.incScoreA();
        else if(find_seq(sBuf, decScoreA))  dispData.decScoreA();
        else if(find_seq(sBuf, incScoreB))  dispData.incScoreB();
        else if(find_seq(sBuf, decScoreB))  dispData.decScoreB();

        else if(find_seq(sBuf, colorChangeA)){
          dispData.nextColorA();
        }
        else if(find_seq(sBuf, colorChangeB)){
          dispData.nextColorB();
        }
        else if(find_seq(sBuf, resetShotclock)){
          dispData.resetShotclock();
        }
#ifdef CROSS_COMPILING
        else if(find_seq(sBuf, start_pause)){
          dispData.start_pause();
        }
#endif
        else
          dispData.SetRefresh(false);

        // Clear buffer after valid sequence/char
        if(dispData.NeedRefresh())
          sBuf.clear(); // = "    ";

        break;
    } // End switch (nInput)
    dispData.UpdateData();
   // printw("%2d %02d:%02d %2d\r", dispData.getScoreA(), dispData.getMin(), dispData.getSec(), dispData.getScoreB());
    dispData.SetRefresh(true);
  }
}
