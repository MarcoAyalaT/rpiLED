#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    char *chipname = "gpiochip0";
    int line_numbers[] = {12};
    int num_lines = sizeof(line_numbers) / sizeof(line_numbers[0]); // Calcular el tamaño de la lista
    struct gpiod_chip *chip;
    struct gpiod_line *line;

    chip = gpiod_chip_open_by_name(chipname);
    
    for (int j = 0; j < 900; j++){
	  for (int i = 0; i < num_lines; i++) {
	    if (j == 0) {
		   line = gpiod_chip_get_line(chip, line_numbers[i]);
		   gpiod_line_request_output(line, "Consumer", 0);
	        }
	    gpiod_line_set_value(line, 1);
	    gpiod_line_set_value(line, 0);	
	    }
    }
    gpiod_chip_close(chip);
    return 0;
}
