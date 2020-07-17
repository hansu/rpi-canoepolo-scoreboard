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

using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}


int main(int argc, char *argv[]) {
  RGBMatrix::Options defaults;
  defaults.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
  defaults.rows = 32;
  defaults.chain_length = 1;
  defaults.parallel = 1;
  defaults.show_refresh_rate = false;
  defaults.pwm_lsb_nanoseconds = 200;
  defaults.brightness = 30;
  defaults.multiplexing = 6;
  defaults.inverse_colors = false;
  defaults.led_rgb_sequence = "BGR";
  char sScore[10];
  char sTime[10];
  int nScoreA=0, nScoreB=0, nPlayTime=600;
  char nInput;
  
  // initializing curses lib
  initscr();
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
  rgb_matrix::Color color(0, 0, 255);
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


  while(1){
    sprintf(sScore, "%2d %2d", nScoreA, nScoreB);
    sprintf(sTime, "%02d:%02d", nPlayTime/60, nPlayTime%60);
    rgb_matrix::DrawText(canvas, font, 0, -1 + font.baseline(), color, outline_font ? NULL : &bg_color, sScore, letter_spacing);
    rgb_matrix::DrawText(canvas, font, 0, 10 + font.baseline(), color, outline_font ? NULL : &bg_color, sTime, letter_spacing);

    // Check input
    nInput = getch();
    switch(nInput){
      case '+': nScoreA++; break;
      case '-': nScoreA--; break;
      case 'r': //nLongPressCnt++;
        nScoreA=0;
        break;
      case 'q':
      case 'x':
        endwin();
        delete canvas;
        return 0;
    }
  }

  //usleep(5000 * 1000);
  canvas->Clear();
  delete canvas;
  return 0;
}
