#include "UmuguMaker.h"

int main(int argc, char **argv) {
    (void)argc;(void)argv;
  umumk::UmuguMaker Editor = umumk::UmuguMaker();
  while (Editor.Update());

  return 0;
}
