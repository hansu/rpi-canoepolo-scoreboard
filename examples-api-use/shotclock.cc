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
#include <thread>
#include <time.h>
#include <iostream>
#include <string>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool bUpdateDisplay = true;
typedef enum states { idle, running, paused } states_t;
typedef enum colors { white, yellow, red, blue, green, orange, violet } colors_t;
#define NUM_COLORS 5


volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}


volatile bool bExit = false;


rgb_matrix::Color color_red(255, 0, 0);
rgb_matrix::Color color_yellow(250, 190, 0);
rgb_matrix::Color color_blue(0, 50, 255);
rgb_matrix::Color color_green(0, 200, 0);
rgb_matrix::Color color_white(200, 200, 200);
rgb_matrix::Color color_orange(250, 130, 0);
rgb_matrix::Color color_violet(220, 0, 220);



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

#ifdef DEBUG
  freopen( "output.txt", "w", stdout );
  cout << "key logging" << endl;
#endif
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

  // It is always good to set up a signal handler to cleanly exit when we
  // receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
  // for that.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);


  rgb_matrix::Color bg_color(0, 0, 0);
  rgb_matrix::Color outline_color(255,255,255);
  int letter_spacing = 0;

  /*
   * Load bdf bitmap fonts.
   */
  rgb_matrix::Font font_std, font_narr;

  fprintf(stderr, "try loading font\n");
  if (!font_std.LoadFont("../fonts2/LiberationSansNarrow_bb32.bdf")) {
    fprintf(stderr, "Couldn't load std font '%s'\n", "../fonts2/LiberationSansNarrow_bb32.bdf");

    if (!font_std.LoadFont("Scoreboard/fonts2/LiberationSansNarrow_bb32.bdf")) {
        fprintf(stderr, "Couldn't load std font '%s'\n", "../fonts2/LiberationSansNarrow_bb32.bdf");
        return 1;
    }
  }

  uint16_t disp_time = 0;
  char disp_time_str[4];


  while(1){

    if(bUpdateDisplay){

      canvas->Clear();

      sprintf(disp_time_str, "%02d ", disp_time);
      rgb_matrix::DrawText(canvas, font_std, 3, 32, *GetPColor(white), &bg_color, disp_time_str, letter_spacing);

      #ifdef CURRENT_TEST
      canvas->Clear();
      for(int i=0; i<32; i++){
       rgb_matrix::DrawLine(canvas, 0, i, 768, i, outline_color);
      }
      #endif

      if (interrupt_received)
        return 0;

      usleep(1 * 1000);  // wait a little to slow down things.

      //  bUpdateDisplay = false;
    }

    usleep(200 * 1000);
  }

  canvas->Clear();
  delete canvas;
  return 0;
}


