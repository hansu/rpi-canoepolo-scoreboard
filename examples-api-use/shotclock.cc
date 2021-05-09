/*
  Shotclock extension for Canoepolo Scoreboard

  Runs on a 48x32 pixel LED-matrix. (Three 16x32 modules in Z-alignment).

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

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

rgb_matrix::Color color_red(255, 0, 0);
rgb_matrix::Color color_yellow(250, 190, 0);
rgb_matrix::Color color_blue(0, 50, 255);
rgb_matrix::Color color_green(0, 200, 0);
rgb_matrix::Color color_white(200, 200, 200);
rgb_matrix::Color color_orange(250, 130, 0);

using namespace std;

Canvas *pCanvas_gl;
Socket *pSocket_gl;

static void InterruptHandler(int signo) {
  if(pCanvas_gl != NULL){
    pCanvas_gl->Clear();
    delete pCanvas_gl;
  }
  if(pSocket_gl != NULL){
    pSocket_gl->Close();
  }
  exit(0);
}


int main(int argc, char *argv[]) {

  rgb_matrix::RuntimeOptions runtime;
  runtime.gpio_slowdown = 4;

  RGBMatrix::Options options;
  options.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
  options.rows = 16;
  options.cols = 32;
  options.chain_length = 3;
  options.parallel = 1;
  options.pixel_mapper_config = "V-mapper:Z;Rotate:90";
  options.show_refresh_rate = false;
  //options.pwm_lsb_nanoseconds = 200;
  options.brightness = 100;
  options.multiplexing = 3;
  options.inverse_colors = false;
  options.led_rgb_sequence = "RGB";


  Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &options);
  if (canvas == NULL)
    return 1;
  pCanvas_gl = canvas;

  // It is always good to set up a signal handler to cleanly exit when we
  // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
  // for that.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);


  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color outline_color(255,255,255);

  /*
   * Load bdf bitmap fonts.
   */
  rgb_matrix::Font font_std, font_narr;

  if (!font_std.LoadFont("../fonts2/LiberationSansNarrow_bb32.bdf")) {
    if (!font_std.LoadFont("Scoreboard/fonts2/LiberationSansNarrow_bb32.bdf")) {
        fprintf(stderr, "Couldn't load std font '%s'\n", "../fonts2/LiberationSansNarrow_bb32.bdf");
        fprintf(stderr, "Couldn't load std font '%s'\n", "Scoreboard/fonts2/LiberationSansNarrow_bb32.bdf");
        return 1;
    }
  }

  rgb_matrix::DrawText(canvas, font_std, 3, 32, color_white, &bg_color, u8"\u2212\u2212"); //--

  // --- Socket ---
  int read_size;
  std::string response;
  Socket csocket(false);
  pSocket_gl = &csocket;

  // Main socket loop
  while(1){
    //Create socket
    if(csocket.SocketCreate() == -1)
    {
      printf("Could not create socket\n");
      return 1;
    }
    printf("Socket is created\n");

    // Connect loop
    while(1){
      // Connect to remote server
      if (csocket.SocketConnect("scoreboard", 9000) < 0) {
          perror("connect failed");
          sleep(2);
          printf("try to reconnect... \n");
      } else {
          printf("Successfully connected to server\n");
          break;
      }
    }

    // Receive loop
    while(1){
      // Receive data from the server
      read_size = csocket.SocketReceive(response);
      if(read_size == 0){
        printf("Connection to server lost\n");
        // this sending is required to not block the port
        csocket.SocketSend("Connection lost, closing socket");
        csocket.Close();
        break;
      }
      printf("send Ack\n");
      if(csocket.SocketSend("Ack") < 0){
        printf("Connection to server lost\n");
        csocket.Close();
        break;
      }

      canvas->Clear();
      rgb_matrix::DrawText(canvas, font_std, 3, 32, color_white, &bg_color, response.c_str());

    }
  }
}

