#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void spline(double x[], double y[], int n, double x0, double *y0) {
    double h[n], alpha[n], l[n+1], mu[n+1], z[n+1];
    double c[n+1], b[n], d[n];
    double a[n+1];
    int i;

    if((x0 < x[0]) || (x0 > x[n-1]))
    {
        printf("Input is out of the range\r\n");
        return;
    }

    for (i = 0; i < n; i++) {
        a[i] = y[i];
    }

    for (i = 0; i < n-1; i++) {
        h[i] = x[i+1] - x[i];
    }

    for (i = 1; i < n-1; i++) {
        alpha[i] = (3.0/h[i]) * (a[i+1] - a[i]) - (3.0/h[i-1]) * (a[i] - a[i-1]);
    }

    l[0] = 1.0;
    mu[0] = 0.0;
    z[0] = 0.0;

    for (i = 1; i < n-1; i++) {
        l[i] = 2.0 * (x[i+1] - x[i-1]) - h[i-1] * mu[i-1];
        mu[i] = h[i] / l[i];
        z[i] = (alpha[i] - h[i-1] * z[i-1]) / l[i];
    }

    l[n-1] = 1.0;
    z[n-1] = 0.0;
    c[n-1] = 0.0;

    for (i = n-2; i >= 0; i--) {
        c[i] = z[i] - mu[i] * c[i+1];
        b[i] = (a[i+1] - a[i]) / h[i] - h[i] * (c[i+1] + 2.0 * c[i]) / 3.0;
        d[i] = (c[i+1] - c[i]) / (3.0 * h[i]);
    }

    for (i = 0; i < n-1; i++) {
        if (x0 >= x[i] && x0 <= x[i+1]) {
            *y0 = a[i] + b[i] * (x0 - x[i]) + c[i] * pow((x0 - x[i]), 2.0) + d[i] * pow((x0 - x[i]), 3.0);
            break;
        }
    }
}

int main() {
    double battery_voltage[] = {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};
    double adc_input[] = {0.572, 0.642, 0.712, 0.785, 0.858, 0.926, 0.997, 1.064, 1.125, 1.193, 1.256, 1.311, 1.364, 1.412, 1.456, 1.501, 1.534, 1.573, 1.605, 1.633, 1.66, 1.685, 1.708, 1.726, 1.748, 1.767, 1.784, 1.801, 1.813};
    int n = sizeof(battery_voltage) / sizeof(battery_voltage[0]);
    double adc_val = 1.5;  // Example value
    double battery_val;

    spline(adc_input, battery_voltage, n, adc_val, &battery_val);
    printf("Interpolated battery voltage: %f,%f\n", adc_val,battery_val);

    adc_val = 1.2;
    spline(adc_input, battery_voltage, n, adc_val, &battery_val);
    printf("Interpolated battery voltage: %f,%f\n", adc_val,battery_val);

    adc_val = 0.3;
    spline(adc_input, battery_voltage, n, adc_val, &battery_val);
    printf("Interpolated battery voltage: %f,%f\n", adc_val,battery_val);

    return 0;
}