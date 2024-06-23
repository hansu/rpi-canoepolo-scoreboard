#include "graphics.h"

struct custom_colors_t {
  custom_colors_t(): bg(0, 0, 0),
                     red(255, 0, 0),
                     yellow(250, 190, 0),
                     yellow_50(125, 85, 0),
                     blue(0, 50, 255),
                     green(0, 200, 0),
                     white(200, 200, 200),
                     white_50(100, 100, 100),
                     orange(250, 130, 0) {}
  rgb_matrix::Color bg;
  rgb_matrix::Color red;
  rgb_matrix::Color yellow;
  rgb_matrix::Color yellow_50;
  rgb_matrix::Color blue;
  rgb_matrix::Color green;
  rgb_matrix::Color white;
  rgb_matrix::Color white_50;
  rgb_matrix::Color orange;
};
