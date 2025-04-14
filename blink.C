#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  char *chipname = "gpiochip0";
  unsigned int line_num = 21;
  struct gpiod_chip *chip;
  struct gpiod_line *line;
  chip = gpiod_chip_open_by_name(chipname);
  printf("Using chip %s\n",chipname);
  line = gpiod_chip_get_line(chip,line_num);
  printf("GPIO line number %u selected\n",line_num);
  gpiod_line_request_output(line,"Consumer",0);
  for (int i = 0; i < 5; i++) {
    gpiod_line_set_value(line,1);
    printf("LED on\n");
    sleep(1);
    gpiod_line_set_value(line,0);
    printf("LED off\n");
    sleep(1);
  }
  gpiod_line_release(line);
  gpiod_chip_close(chip);
  return 0;
}




