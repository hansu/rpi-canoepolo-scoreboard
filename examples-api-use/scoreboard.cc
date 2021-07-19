/*
  Canoepolo Scoreboard

  Runs on a 160x32 pixel LED-matrix. (Two rows of each five 16x32 modules).

  Copyright 2021 Hans Unzner (hansunzner@gmail.com)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 */


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
extern "C"{
#include "ws.h"
}

//#define CROSS_COMPILING

/* use this if using a cross compiler because it doesn't have ncurses lib included */
#ifndef CROSS_COMPILING
/* Set this define if ncurses lib is not available.
It is used for direct action on keyboard input without the need to press return */
#define USE_NCURSES
#endif

#ifdef USE_NCURSES
#include <curses.h>
#endif

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

char sScoreA[24], sScoreB[24], sTime[24];


volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

void KeyboardInput(DisplayData& dispData);
void ShotclockCom1 (DisplayData& dispData);
void ShotclockCom2 (DisplayData& dispData);
void WsSocket (DisplayData& dispData);
volatile bool bExit = false;


rgb_matrix::Color color_red(255, 0, 0);
rgb_matrix::Color color_yellow(250, 190, 0);
rgb_matrix::Color color_blue(0, 50, 255);
rgb_matrix::Color color_green(0, 200, 0);
rgb_matrix::Color color_white(200, 200, 200);
rgb_matrix::Color color_orange(250, 130, 0);
rgb_matrix::Color color_violet(220, 0, 220);

rgb_matrix::Color* pTimeColor;
DisplayData dispData;


rgb_matrix::Color* GetPColor(colors_t nColorIndex){
  switch(nColorIndex){
    case red:    return &color_red;
    case yellow: return &color_yellow;
    case blue:   return &color_blue;
    case green:  return &color_green;
    case orange: return &color_orange;
    case violet: return &color_violet;
    case white:
    default:
                 return &color_white;
    }
}


using namespace std;

int main(int argc, char *argv[]) {

  rgb_matrix::RuntimeOptions runtime;
  runtime.gpio_slowdown = 4;

#ifdef DEBUG_KEYS
  freopen( "output.txt", "w", stdout );
  cout << "key logging" << endl;
#endif
  RGBMatrix::Options options;
  options.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
  options.rows = 16;
  options.cols = 32;
  options.chain_length = 5;
  options.parallel = 2;
  //options.pixel_mapper_config = "V-mapper:Z;Rotate:90";
  options.show_refresh_rate = false;
  //options.pwm_lsb_nanoseconds = 200;
  options.brightness = 100;
  options.multiplexing = 3;
  options.inverse_colors = false;
  options.led_rgb_sequence = "RGB";

#ifdef USE_NCURSES
  // initializing curses lib
  initscr();
  noecho();
  timeout(-1); // set to blocking mode - otherwise time out value
#endif

  Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &options);
  if (canvas == NULL)
    return 1;

  // It is always good to set up a signal handler to cleanly exit when we
  // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
  // for that.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);


  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color outline_color(255,255,255);
  int letter_spacing = 0;

  // Load bdf bitmap fonts
  rgb_matrix::Font font_std, font_narr;
  if (!font_std.LoadFont("../fonts2/LiberationSansNarrow_bb32.bdf")) {
    if (!font_std.LoadFont("Scoreboard/fonts2/LiberationSansNarrow_bb32.bdf")) {
      fprintf(stderr, "Couldn't load std font '%s'\n", "../fonts2/LiberationSansNarrow_bb32.bdf");
      fprintf(stderr, "Couldn't load std font '%s'\n", "Scoreboard/fonts2/LiberationSansNarrow_bb32.bdf");
      return 1;
    }
  }
  if (!font_narr.LoadFont("../fonts2/antonio_b32.bdf")) {
    if (!font_narr.LoadFont("Scoreboard/fonts2/antonio_b32.bdf")) {
      fprintf(stderr, "Couldn't load narrow font '%s'\n", "../fonts2/antonio_b32.bdf");
      fprintf(stderr, "Couldn't load narrow font '%s'\n", "Scoreboard/fonts2/antonio_b32.bdf");
      return 1;
    }
  }

  pTimeColor = &color_white;;

  std::thread inputThread(KeyboardInput, std::ref(dispData));
  std::thread socketThread1(ShotclockCom1, std::ref(dispData));
  std::thread socketThread2(ShotclockCom2, std::ref(dispData));
  std::thread wsSocketThread(WsSocket, std::ref(dispData));

  while(1){
    dispData.updateTime();

    if(dispData.NeedRefresh()){

      canvas->Clear();

      if(dispData.getScoreA() < 10){
        sprintf(sScoreA, "%d ", dispData.getScoreA());
        rgb_matrix::DrawText(canvas, font_std, 0, 32, *GetPColor(dispData.getColorIndexA()), &bg_color, sScoreA, letter_spacing);
      } else{
        sprintf(sScoreA, "%2d ", dispData.getScoreA());
        rgb_matrix::DrawText(canvas, font_narr, 0, 32, *GetPColor(dispData.getColorIndexA()), &bg_color, sScoreA, letter_spacing);
      }

      sprintf(sTime, "%2d:%02d", dispData.getMin(), dispData.getSec());

      if(dispData.getMin() == 1)
        rgb_matrix::DrawText(canvas, font_std, 35, 32, *pTimeColor,  &bg_color, sTime,   letter_spacing);
      else if(dispData.getMin() < 10)
        rgb_matrix::DrawText(canvas, font_std, 29, 32, *pTimeColor,  &bg_color, sTime,   letter_spacing);
      else if(dispData.getMin() < 20)
        rgb_matrix::DrawText(canvas, font_std, 36, 32, *pTimeColor,  &bg_color, sTime,   letter_spacing);
      else
        rgb_matrix::DrawText(canvas, font_std, 33, 32, *pTimeColor,  &bg_color, sTime,   letter_spacing);

      if(dispData.getScoreB() < 10){
        sprintf(sScoreB, "%d", dispData.getScoreB());
        rgb_matrix::DrawText(canvas, font_std, 139, 32, *GetPColor(dispData.getColorIndexB()), &bg_color, sScoreB, letter_spacing);
      } else {
        sprintf(sScoreB, "%2d", dispData.getScoreB());
        if(dispData.getScoreB() < 20)
          rgb_matrix::DrawText(canvas, font_narr, 128+5, 32, *GetPColor(dispData.getColorIndexB()), &bg_color, sScoreB, letter_spacing);
        else
          rgb_matrix::DrawText(canvas, font_narr, 128, 32, *GetPColor(dispData.getColorIndexB()), &bg_color, sScoreB, letter_spacing);
      }

      #ifdef CURRENT_TEST
      canvas->Clear();
      for(int i=0; i<32; i++){
       rgb_matrix::DrawLine(canvas, 0, i, 768, i, outline_color);
      }
      #endif
      dispData.SetRefresh(false);
    }
    if(bExit)
    {
      break;
    }
    usleep(200 * 1000);
  }
#ifdef USE_NCURSES
  endwin();
#endif
  canvas->Clear();
  delete canvas;
  return 0;
}

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
#ifdef USE_NCURSES
    nInput = getch();
#else
    nInput = getchar();
#endif
#ifdef DEBUG_KEYS
    cout << nInput << "-------" << endl;
#endif
    switch(nInput){
      case 'r':
        nShutdownCnt++;
        if(nShutdownCnt > 150)
        {
          //system("echo raspberry | sudo -S poweroff");
          system("sudo shutdown now");
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
          dispData.nextColorIndexA();
        }
        else if(find_seq(sBuf, colorChangeB)){
          dispData.nextColorIndexB();
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
    }
   // printw("%2d %02d:%02d %2d\r", dispData.getScoreA(), dispData.getMin(), dispData.getSec(), dispData.getScoreB());
    dispData.SetRefresh(true);
  }
}

void ShotclockCom1 (DisplayData& dispData)
{
  Socket csocket(true);

 //Create socket
  while(1){
    if(csocket.SocketCreate() == -1)
    {
      printf("Could not create socket\n");
      sleep(5);
    } else{
      printf("Socket is created\n");
      break;
    }
  }

  //Bind
  while(1){
    if(csocket.BindCreatedSocket(9000) < 0)
    {
      printf("bind failed");
      sleep(5);
    } else {
      printf("bind done\n");
      break;
    }
  }

  //Listen
  csocket.Listen();
  char tx_string[10];
  int retVal;

  while(1)
  {
      printf("Waiting for incoming connections...\n");
      //Accept incoming connection
      if (csocket.Accept() < 0)
      {
          perror("accept failed");
          break;
      }
      printf("Connection accepted\n");

      while(1){
        //if(dispData.NeedShotclockRefresh()){
        sprintf(tx_string, "%02d", dispData.getShotTimeout());
        printf("send %s\n", tx_string);

        if((retVal = csocket.SocketSend(std::string(tx_string))) < 0)
        {
          printf("send failed (error %d)\n", retVal);
          break;
        }
        usleep(250000);
      }
  }

}

void ShotclockCom2 (DisplayData& dispData)
{
  Socket csocket(true);

 //Create socket
  while(1){
    if(csocket.SocketCreate() == -1)
    {
      printf("[2] Could not create socket\n");
      sleep(5);
    } else{
      printf("[2] Socket is created\n");
      break;
    }
  }

  //Bind
  while(1){
    if(csocket.BindCreatedSocket(9001) < 0)
    {
      printf("[2] bind failed");
      sleep(5);
    } else {
      printf("[2] bind done\n");
      break;
    }
  }

  //Listen
  csocket.Listen();
  char tx_string[10];
  int retVal;

  while(1)
  {
      printf("[2] Waiting for incoming connections...\n");
      //Accept incoming connection
      if (csocket.Accept() < 0)
      {
          perror("[2] accept failed");
          break;
      }
      printf("[2] Connection accepted\n");

      while(1){
        //if(dispData.NeedShotclockRefresh()){
        sprintf(tx_string, "%02d", dispData.getShotTimeout());
        printf("[2] send %s\n", tx_string);

        if((retVal = csocket.SocketSend(std::string(tx_string))) < 0)
        {
          printf("[2] send failed (error %d)\n", retVal);
          break;
        }
        usleep(250000);
      }
  }

}

void onopen(int fd)
{
  char *cli;
  cli = ws_getaddress(fd);
  printf("Connection opened, client: %d | addr: %s\n", fd, cli);

  free(cli);
}

void onclose(int fd)
{
  char *cli;
  cli = ws_getaddress(fd);
  printf("Connection closed, client: %d | addr: %s\n", fd, cli);
  free(cli);
}

void onmessage(int fd, const unsigned char *msg, uint64_t size, int type)
{
  char *cli;
  std::stringstream ssResponse;
  cli = ws_getaddress(fd);
  printf("Received message: %s (size: %" PRId64 ", type: %d), from: %s/%d  -->  ",
    msg, size, type, cli, fd);
  free(cli);


  if(strstr((const char*)msg,"update") != NULL){
    // do nothing, just send data
  } else {
    if(strstr((const char*)msg,"playPause") != NULL){
      dispData.start_pause();
    } else if(strstr((const char*)msg,"shotclockReset") != NULL){
      dispData.resetShotclock();
    } else if(strstr((const char*)msg,"scoreLeftPlus") != NULL){
      dispData.incScoreA();
    } else if(strstr((const char*)msg,"scoreRightPlus") != NULL){
      dispData.incScoreB();
    } else if(strstr((const char*)msg,"scoreLeftMinus") != NULL){
      dispData.decScoreA();
    } else if(strstr((const char*)msg,"scoreRightMinus") != NULL){
      dispData.decScoreB();
    } else if(strstr((const char*)msg,"reset") != NULL){
      dispData.resetScore();
      dispData.stopTimer();
      dispData.setTime(600);
      dispData.resetColors();
      dispData.resetShotclock();
    } else if(strstr((const char*)msg,"switch") != NULL){
      dispData.swapTeams();
    } else if(strstr((const char*)msg,"colorLeft") != NULL){
      dispData.nextColorIndexA();
    } else if(strstr((const char*)msg,"colorRight") != NULL){
      dispData.nextColorIndexB();
    } else if(strstr((const char*)msg,"timePlus") != NULL){
      dispData.modifyTime(60);
    } else if(strstr((const char*)msg,"timeMinus") != NULL){
      dispData.modifyTime(-60);
    }
    dispData.SetRefresh(true);
  }

  // {"time" : [10, 0],"shotclock" : 60,"score" : [0, 0]};
  ssResponse << "{\"time\" : [" << dispData.getMin() << "," << dispData.getSec() << "],\"shotclock\" : " << dispData.getShotTimeout() << ",\"score\" : [" << dispData.getScoreA()  << "," << dispData.getScoreB() << "]}";
  std::cout << "send: " << ssResponse.str() << std::endl;
  ws_sendframe(fd, ssResponse.str().c_str() , ssResponse.str().size(), true, type);
}

void WsSocket (DisplayData& dispData){

  struct ws_events evs;
  evs.onopen    = &onopen;
  evs.onclose   = &onclose;
  evs.onmessage = &onmessage;
  ws_socket(&evs, 8080); /* Never returns. */


}
