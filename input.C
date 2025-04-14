#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
int main(int argc, char **argv)
{
	char *chipname = "gpiochip0";
	unsigned int line_num = 13;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int old, new_val;
	int n=0;
	
	chip = gpiod_chip_open_by_name(chipname);
	line = gpiod_chip_get_line(chip,line_num);

	gpiod_line_request_input(line,"Consumer");
	old = gpiod_line_get_value(line);
	//printf("State is %d\n",old);
	for (int t=0; t <= 9999999; t++) {
		new_val = gpiod_line_get_value(line);
		if (new_val != old) {
		  //printf("State changed to %d\n",new_val);
			old = new_val;
			n++;
			t++;
		}
		else {
		  t++;
		}
	}
	printf("Cambios totales: %d\n", n);
	gpiod_line_release(line);
	gpiod_chip_close(chip);
	return 0;
}
