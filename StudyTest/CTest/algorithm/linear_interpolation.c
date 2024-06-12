#include <stdio.h>

float linear_interpolation(float adc_input[], float battery_voltage[], int size, float input) {
    for (int i = 0; i < size - 1; i++) {
        if (input >= adc_input[i] && input <= adc_input[i + 1]) {
            float slope = (battery_voltage[i + 1] - battery_voltage[i]) / (adc_input[i + 1] - adc_input[i]);
            return battery_voltage[i] + slope * (input - adc_input[i]);
        }
    }
    // If input is out of range, return -1 or handle as needed
    return -1;
}

int main() {
    float battery_voltage[] = {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};
    float adc_input[] = {0.572, 0.642, 0.712, 0.785, 0.858, 0.926, 0.997, 1.064, 1.125, 1.193, 1.256, 1.311, 1.364, 1.412, 1.456, 1.501, 1.534, 1.573, 1.605, 1.633, 1.66, 1.685, 1.708, 1.726, 1.748, 1.767, 1.784, 1.801, 1.813};
    int size = sizeof(adc_input) / sizeof(adc_input[0]);
    float input = 1.5; // Example ADC input value

    float result = linear_interpolation(adc_input, battery_voltage, size, input);
    if (result != -1) {
        printf("The corresponding battery voltage is: %f,%f\n", input,result);
    } else {
        printf("Input is out of range.\n");
    }

    input = 1.2; // Example ADC input value
    result = linear_interpolation(adc_input, battery_voltage, size, input);
    if (result != -1) {
        printf("The corresponding battery voltage is: %f,%f\n", input,result);
    } else {
        printf("Input is out of range.\n");
    }

    return 0;
}