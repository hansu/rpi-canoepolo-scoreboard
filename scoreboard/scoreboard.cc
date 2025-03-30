
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
// #define USE_NCURSES
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
  printf("exit...\n\r");
}

void KeyboardInput(DisplayData& dispData);
void ShotclockCom1 (DisplayData& dispData);
void ShotclockCom2 (DisplayData& dispData);
void WsSocket (DisplayData& dispData);
volatile bool bExit = false;

rgb_matrix::Color* pTimeColor;
DisplayData dispData;

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
  printf("Scoreboard started. Exit with q\n");
#else
  printf("Scoreboard started. Exit with q and Enter\n\r");
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

  rgb_matrix::Color color_white(200, 200, 200);
  pTimeColor = &color_white;

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
        rgb_matrix::DrawText(canvas, font_std, 0, 32, *dispData.getColorA(), &bg_color, sScoreA, letter_spacing);
      } else{
        sprintf(sScoreA, "%2d ", dispData.getScoreA());
        rgb_matrix::DrawText(canvas, font_narr, 0, 32, *dispData.getColorA(), &bg_color, sScoreA, letter_spacing);
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
        rgb_matrix::DrawText(canvas, font_std, 139, 32, *dispData.getColorB(), &bg_color, sScoreB, letter_spacing);
      } else {
        sprintf(sScoreB, "%2d", dispData.getScoreB());
        if(dispData.getScoreB() < 20)
          rgb_matrix::DrawText(canvas, font_narr, 128+5, 32, *dispData.getColorB(), &bg_color, sScoreB, letter_spacing);
        else
          rgb_matrix::DrawText(canvas, font_narr, 128, 32, *dispData.getColorB(), &bg_color, sScoreB, letter_spacing);
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

/**
 * @brief This function is called whenever a new websocketconnection is opened.
 * @param fd The new client file descriptor.
 */
void onopen(int fd)
{
  char *cli;
  cli = ws_getaddress(fd);
  printf("Connection opened, client: %d | addr: %s\n\r", fd, cli);
  dispData.AddWebsocketClient(fd);
  dispData.SendWebsocketData(fd);
  free(cli);
}

/**
 * @brief This function is called whenever a websocket connection is closed.
 * @param fd The client file descriptor.
 */
void onclose(int fd)
{
  char *cli;
  cli = ws_getaddress(fd);
  printf("Connection closed, client: %d | addr: %s\n\r", fd, cli);
  dispData.RemoveWebsocketClient(fd);
  free(cli);
}

/*
 * Function to split a string by a delimiter and return a vector of substrings
 */
std::vector<std::string> split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

/*
 * Websocket message events goes here.
 * @param fd   Client file descriptor.
 * @param msg  Message content.
 * @param size Message size.
 * @param type Message type (text or binay).
 */
void onmessage(int fd, const unsigned char *msg, uint64_t size, int type)
{
  char *cli;
  cli = ws_getaddress(fd);
  printf("Received message: %s (size: %" PRId64 ", type: %d), from: %s/%d --> ", msg, size, type, cli, fd);
  free(cli);

  if(size == 0) return;

  // Expected string format: "ColorL=0,0,0;ColorR=0,0,0;ScoreL=0;ScoreR=0;Time=0;Shotclock=60";
  std::string input(reinterpret_cast<const char *>(msg));
  // Split the input string by semicolons to get key-value pairs
  std::vector<std::string> pairs = split(input, ';');
  try {
    // Iterate over each pair
    for (const auto &pair : pairs) {
      // Split each pair by the equals sign to separate key and value
      std::vector<std::string> keyValue = split(pair, '=');
      if (keyValue.size() == 2) {
        if(keyValue[0] == "Time") dispData.setTime(stoi(keyValue[1]));
        else if(keyValue[0] == "Shotclock") dispData.setShotclockTime(stoi(keyValue[1]));
        else if(keyValue[0] == "ScoreL") dispData.setScoreA(stoi(keyValue[1]));
        else if(keyValue[0] == "ScoreR") dispData.setScoreB(stoi(keyValue[1]));
        else if(keyValue[0] == "ColorL") {
          std::vector<std::string> colorRGB = split(keyValue[1], ',');
          printf("Color L: %d %d %d\n\r", stoi(colorRGB[0]), stoi(colorRGB[1]), stoi(colorRGB[2]));
          dispData.setColorA_RGB(stoi(colorRGB[0]), stoi(colorRGB[1]), stoi(colorRGB[2]));
        }
        else if(keyValue[0] == "ColorR") {
          std::vector<std::string> colorRGB = split(keyValue[1], ',');
          printf("Color R: %d %d %d\n\r", stoi(colorRGB[0]), stoi(colorRGB[1]), stoi(colorRGB[2]));
          dispData.setColorB_RGB(stoi(colorRGB[0]), stoi(colorRGB[1]), stoi(colorRGB[2]));
        }
      }
    }
  } catch(const std::invalid_argument &ex) {
    printf("Error while converting number! (stoi)\n");
  } catch(...) {
    printf("Error while parsing string!\n");
  }

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
      dispData.nextColorA();
    } else if(strstr((const char*)msg,"colorRight") != NULL){
      dispData.nextColorB();
    } else if(strstr((const char*)msg,"timeMinutePlus") != NULL){
      dispData.modifyTime(60);
    } else if(strstr((const char*)msg,"timeMinuteMinus") != NULL){
      dispData.modifyTime(-60);
    } else if(strstr((const char*)msg,"timeSecondPlus") != NULL){
      dispData.modifyTime(1);
    } else if(strstr((const char*)msg,"timeSecondMinus") != NULL){
      dispData.modifyTime(-1);
    }
    dispData.SetRefresh(true);
  }
  dispData.SendWebsocketData(fd);
}

/*
 * Sends data to the specified websocket connection.
 * Format: {"time" : [10, 0],"shotclock" : 60,"score" : [0, 0], state = "idle"};
 */
void DisplayData::SendWebsocketData(int fd){
    std::stringstream ssResponse;
    ssResponse << "{\"time\" : [" << dispData.getMin() << "," << dispData.getSec() << \
    "],\"shotclock\" : " << dispData.getShotTimeout() << ",\"score\" : [" << dispData.getScoreA() \
    << "," << dispData.getScoreB() << "],\"state\" : \"" << dispData.state2str(dispData.getState()) << "\"}";
    std::cout << "send: " << ssResponse.str() << ", (state: " << dispData.state2str(dispData.getState()) << \
    ", shotclockState: " << dispData.state2str(dispData.getShotclockState()) << ")" <<std::endl;
    ws_sendframe(fd, ssResponse.str().c_str() , ssResponse.str().size(), true, 1);
  }

/*
* Register websocket events.
*/
void WsSocket (DisplayData& dispData){
  struct ws_events evs;
  evs.onopen    = &onopen;
  evs.onclose   = &onclose;
  evs.onmessage = &onmessage;
  ws_socket(&evs, 8080); /* Never returns. */
}
