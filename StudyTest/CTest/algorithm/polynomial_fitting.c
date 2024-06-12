#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DEGREE 5

double battery_voltage[] = {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};
double adc_input[] = {0.572, 0.642, 0.712, 0.785, 0.858, 0.926, 0.997, 1.064, 1.125, 1.193, 1.256, 1.311, 1.364, 1.412, 1.456, 1.501, 1.534, 1.573, 1.605, 1.633, 1.66, 1.685, 1.708, 1.726, 1.748, 1.767, 1.784, 1.801, 1.813};
int n = sizeof(battery_voltage) / sizeof(battery_voltage[0]);
double coeff[DEGREE + 1];

void polyfit(double* x, double* y, int n, int degree, double* coeff) {
    int i, j, k;
    double* X = (double*)malloc((2 * degree + 1) * sizeof(double));
    double* B = (double*)malloc((degree + 1) * sizeof(double));
    double** A = (double**)malloc((degree + 1) * sizeof(double*));
    for (i = 0; i < degree + 1; i++) {
        A[i] = (double*)malloc((degree + 1) * sizeof(double));
    }

    for (i = 0; i < 2 * degree + 1; i++) {
        X[i] = 0;
        for (j = 0; j < n; j++) {
            X[i] += pow(x[j], i);
        }
    }

    for (i = 0; i < degree + 1; i++) {
        for (j = 0; j < degree + 1; j++) {
            A[i][j] = X[i + j];
        }
    }

    for (i = 0; i < degree + 1; i++) {
        B[i] = 0;
        for (j = 0; j < n; j++) {
            B[i] += pow(x[j], i) * y[j];
        }
    }

    for (i = 0; i < degree + 1; i++) {
        for (k = i + 1; k < degree + 1; k++) {
            if (A[i][i] < A[k][i]) {
                for (j = 0; j < degree + 1; j++) {
                    double temp = A[i][j];
                    A[i][j] = A[k][j];
                    A[k][j] = temp;
                }
                double temp = B[i];
                B[i] = B[k];
                B[k] = temp;
            }
        }
    }

    for (i = 0; i < degree + 1; i++) {
        for (k = i + 1; k < degree + 1; k++) {
            double t = A[k][i] / A[i][i];
            for (j = 0; j < degree + 1; j++) {
                A[k][j] -= t * A[i][j];
            }
            B[k] -= t * B[i];
        }
    }

    for (i = degree; i >= 0; i--) {
        coeff[i] = B[i];
        for (j = i + 1; j < degree + 1; j++) {
            coeff[i] -= A[i][j] * coeff[j];
        }
        coeff[i] /= A[i][i];
    }

    free(X);
    free(B);
    for (i = 0; i < degree + 1; i++) {
        free(A[i]);
    }
    free(A);
}

double polyval(double* coeff, int degree, double x) {
    double result = 0;

    if((x < adc_input[0]) || (x > adc_input[n-1]))
    {
        printf("Input is out of the range\r\n");
        return -1;
    }

    for (int i = degree; i >= 0; i--) {
        result = result * x + coeff[i];
    }
    return result;
}

int main() {
    polyfit(adc_input, battery_voltage, n, DEGREE, coeff);

    double input = 1.5;
    double output = polyval(coeff, DEGREE, input);
    printf("For adc_input = %f, the battery_voltage = %f\n", input, output);

    input = 1.2;
    output = polyval(coeff, DEGREE, input);
    printf("For adc_input = %f, the battery_voltage = %f\n", input, output);

    input = 0.3;
    output = polyval(coeff, DEGREE, input);
    printf("For adc_input = %f, the battery_voltage = %f\n", input, output);

    return 0;
}