#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>
#define MAX_LEN 128

typedef struct {
    int Imaginary_part;
    int Real_part;
} complex_num;

typedef struct {
    int dim;
    int** matrix1;
    int** matrix2;
    char operation[MAX_LEN];
}input_int;

typedef struct {
    int dim;
    double** matrix1;
    double** matrix2;
    char operation[MAX_LEN];
}input_double;

typedef struct {
    int dim;
    complex_num ** matrix1;
    complex_num ** matrix2;
    char operation[MAX_LEN];
}input_complex;

complex_num add_complex(complex_num num1,complex_num num2){
    complex_num res;
    res.Imaginary_part=num1.Imaginary_part+num2.Imaginary_part;
    res.Real_part=num1.Real_part+num2.Real_part;
    return res;
}
complex_num mul_complex(complex_num num1, complex_num num2){
    complex_num res;
    res.Imaginary_part=(num1.Imaginary_part * num2.Real_part) + (num1.Real_part * num2.Imaginary_part);
    res.Real_part=(num1.Real_part * num2.Real_part) - (num1.Imaginary_part * num2.Imaginary_part);
    return res;
}
void parse_complex(char* token, complex_num * value) {
    char temp[strlen(token) + 1];
    int i = 0, j = 0;
    char sign = 0;

    // Check for leading '-'
    if (token[i] == '-') {
        i++;
        sign = '-';
    }

    // Copy the real part or imaginary part before the first '+' or '-' or 'i'
    while (token[i] != '+' && token[i] != '-' && token[i] != 'i' && token[i] != '\0') {
        temp[j++] = token[i++];
    }
    temp[j] = '\0';

    if (token[i] == 'i') {
        // Case bi or -bi
        if (j == 0) {
            // case "i" or "-i"
            value->Imaginary_part = 1;
        } else {
            sscanf(temp, "%d", &value->Imaginary_part);
        }
        value->Real_part = 0;
        if (sign == '-') {
            value->Imaginary_part *= -1;
        }
    } else {
        sscanf(temp, "%d", &value->Real_part);
        if (sign == '-') {
            value->Real_part *= -1;
        }
        if (token[i] == 'i') {
            // Case a+i or a-i
            value->Imaginary_part = (sign == '-') ? -1 : 1;
        } else if (token[i] == '+' || token[i] == '-') {
            sign = token[i++];
            j = 0;
            // Copy the imaginary part
            while (token[i] != 'i' && token[i] != '\0') {
                temp[j++] = token[i++];
            }
            temp[j] = '\0';

            if (token[i] == 'i') {
                sscanf(temp, "%d", &value->Imaginary_part);
                if (sign == '-') {
                    value->Imaginary_part *= -1;
                }
            } else {
                // Case a
                value->Imaginary_part = 0;
            }
        } else {
            // Case a
            value->Imaginary_part = 0;
        }
    }
}
double rou (double x){
    x= round(x*10) /10;
    return x;
}
bool is_complex(const char* str) {
    return strchr(str, 'i') != NULL;
}
int is_double(const char* str) {
    return strchr(str, '.') != NULL;
}


void fill_matrix_complex(const char* input, complex_num*** matrix, int rows, int cols) {
    *matrix = (complex_num**)malloc((rows) * sizeof(complex_num*));
    for (int i = 0; i < rows; i++) {
        (*matrix)[i] = (complex_num*)malloc((cols) * sizeof(complex_num));
    }
    const char* values_start = strchr(input, ':') + 1;

    char* values_copy = strdup(values_start);  // Make a modifiable copy of the values string
    char* token = strtok(values_copy, ",");
    int k = 0;
    int count =0;
    while (token != NULL) {
        count++;
        complex_num value = {0, 0};
        parse_complex(token,&value);

        (*matrix)[k / (cols)][k % (cols)] = value;
        k++;
        token = strtok(NULL, ",");
    }
    free(values_copy);
}
void fill_matrix_double(const char* input, double*** matrix, int rows, int cols) {
    *matrix = (double**)malloc((rows) * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        (*matrix)[i] = (double*)malloc((cols) * sizeof(double));
    }

    const char* values_start = strchr(input, ':') + 1;

    char* values_copy = strdup(values_start);  // Make a modifiable copy of the values string
    char* token = strtok(values_copy, ",");
    int k = 0;
    int count=0;
    while (token != NULL) {
        double value;
        count++;
        sscanf(token, "%lf", &value);
        value = rou(value);
        (*matrix)[k / (cols)][k % (cols)] = value;
        k++;
        token = strtok(NULL, ",");
    }
    free(values_copy);
}
void fill_matrix_int(const char* input, int*** matrix, int rows, int cols) {
    *matrix = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        (*matrix)[i] = (int*)malloc(cols * sizeof(int));
    }

    const char* values_start = strchr(input, ':') + 1;
    char* values_copy = strdup(values_start);  // Make a modifiable copy of the values string
    char* token = strtok(values_copy, ",");
    int k = 0;
    int count=0;
    while (token != NULL) {
        int value;
        count++;
        sscanf(token, "%d", &value);
        (*matrix)[k / (cols)][k % (cols)] = value;
        k++;
        token = strtok(NULL, ",");
    }
    free(values_copy);
}

complex_num** add_mul_complex(char* operation, complex_num** first_matrix, complex_num** second_matrix, int dim) {
    complex_num** output = (complex_num**)malloc(dim * dim * sizeof(complex_num));

    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < dim; i++) {
            output[i] = (complex_num*)malloc(dim * sizeof(complex_num));
            for (int j = 0; j < dim; j++) {
                output[i][j] = add_complex(first_matrix[i][j], second_matrix[i][j]);
            }
        }
        return output;
    }  if (strcmp(operation, "MUL")==0) {
        for (int i = 0; i < dim; i++) {
            output[i] = (complex_num*)malloc(dim * sizeof(complex_num));
            for (int j = 0; j < dim; j++) {
                output[i][j].Imaginary_part = 0;
                output[i][j].Real_part = 0;
            }
        }
        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                for (int k = 0; k < dim; k++) {
                    complex_num product = mul_complex(first_matrix[i][k], second_matrix[k][j]);
                    output[i][j] = add_complex(output[i][j], product);
                }
            }
        }

        return output;
    }

    return NULL;

}
double** add_mul_double(char* operation, double** first_matrix, double** second_matrix, int dim) {
    double ** output = (double **)malloc(dim * dim*  sizeof(double ));
    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < dim; i++) {
            output[i] = (double*)malloc(dim * sizeof(double));
            for (int j = 0; j < dim; j++) {
                output[i][j] = first_matrix[i][j] + second_matrix[i][j];
            }
        }
        return output;

    }  if (strcmp(operation, "MUL")==0) {

        for (int i = 0; i < dim; i++) {
            output[i] = (double *)malloc(dim * sizeof(double ));
        }

        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j] = 0;
                for (int k = 0; k < dim; k++) {
                    output[i][j] = rou(output[i][j]) + rou(first_matrix[i][k] * second_matrix[k][j]);
                }
            }
        }

        return output;
    }
    return NULL;
}
int** add_mul_int(char* operation, int** first_matrix, int** second_matrix, int dim) {
    int** output = (int**)malloc(dim *  dim * sizeof(int));
    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < dim; i++) {
            output[i] = (int*)malloc(dim * sizeof(int));
            for (int j = 0; j < dim; j++) {
                output[i][j] = first_matrix[i][j] + second_matrix[i][j];
            }
        }
        return output;

    }  if (strcmp(operation, "MUL")==0){
        for (int i = 0; i < dim; i++) {
            output[i] = (int*)malloc(dim * sizeof(int));
        }

        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j] = 0;
                for (int k = 0; k < dim; k++) {
                    output[i][j] += first_matrix[i][k] * second_matrix[k][j];
                }
            }
        }

        return output;
    }
    return NULL;
}

int** add_mul_all_matrices_int(char* operation, int*** matrices, int dim, int num_of_matrices) {
    int** output = (int**)malloc(dim * sizeof(int*));
    for (int i = 0; i < dim; i++) {
        output[i] = (int*)malloc(dim * sizeof(int));
    }

    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j] = 0;
                for (int k = 0; k < num_of_matrices; k++) {
                    output[i][j] += matrices[k][i][j];
                }
            }
        }
    } else if (strcmp(operation, "MUL") == 0) {
        int** temp = (int**)malloc(dim * sizeof(int*));
        for (int i = 0; i < dim; i++) {
            temp[i] = (int*)malloc(dim * sizeof(int));
        }

        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j] = matrices[0][i][j];
            }
        }

        for (int k = 1; k < num_of_matrices; k++) {
            for (int i = 0; i < dim; i++) {
                for (int j = 0; j < dim; j++) {
                    temp[i][j] = 0;
                    for (int l = 0; l < dim; l++) {
                        temp[i][j] += output[i][l] * matrices[k][l][j];
                    }
                }
            }

            for (int i = 0; i < dim; i++) {
                for (int j = 0; j < dim; j++) {
                    output[i][j] = temp[i][j];
                }
            }
        }
        for (int i = 0; i < dim; i++) {
            free(temp[i]);
        }
        free(temp);
    }
    return output;
}
double** add_mul_all_matrices_double(char* operation, double*** matrices, int dim, int num_of_matrices) {
    double** output = (double**)malloc(dim * sizeof(double*));
    for (int i = 0; i < dim; i++) {
        output[i] = (double*)malloc(dim * sizeof(double));
    }

    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j] = 0.0;
                for (int k = 0; k < num_of_matrices; k++) {
                    output[i][j] = rou(output[i][j]) + rou(matrices[k][i][j]);
                }
            }
        }
    } if (strcmp(operation, "MUL") == 0) {
        double** temp = (double**)malloc(dim * sizeof(double*));
        for (int i = 0; i < dim; i++) {
            temp[i] = (double*)malloc(dim * sizeof(double));
        }

        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j] = matrices[0][i][j];
            }
        }

        for (int k = 1; k < num_of_matrices; k++) {
            for (int i = 0; i < dim; i++) {
                for (int j = 0; j < dim; j++) {
                    temp[i][j] = 0.0;
                    for (int l = 0; l < dim; l++) {
                        temp[i][j] = rou(temp[i][j]) + rou(output[i][l] * matrices[k][l][j]);
                    }
                }
            }

            for (int i = 0; i < dim; i++) {
                for (int j = 0; j < dim; j++) {
                    output[i][j] = temp[i][j];
                }
            }
        }

        for (int i = 0; i < dim; i++) {
            free(temp[i]);
        }
        free(temp);
    }

    return output;
}
complex_num** add_mul_all_matrices_complex(char* operation, complex_num*** matrices, int dim, int num_of_matrices) {
    complex_num **output = (complex_num **) malloc(dim * sizeof(complex_num *));
    for (int i = 0; i < dim; i++) {
        output[i] = (complex_num *) malloc(dim * sizeof(complex_num));
    }

    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j].Imaginary_part = 0;
                output[i][j].Real_part = 0;
                for (int k = 0; k < num_of_matrices; k++) {
                    output[i][j] = add_complex(output[i][j], matrices[k][i][j]);
                }
            }
        }
    } if (strcmp(operation, "MUL") == 0) {
        complex_num **temp = (complex_num **) malloc(dim * sizeof(complex_num *));
        for (int i = 0; i < dim; i++) {
            temp[i] = (complex_num *) malloc(dim * sizeof(complex_num));
        }

        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j] = matrices[0][i][j];
            }
        }

        for (int k = 1; k < num_of_matrices; k++) {
            for (int i = 0; i < dim; i++) {
                for (int j = 0; j < dim; j++) {
                    temp[i][j].Imaginary_part = 0;
                    temp[i][j].Real_part = 0;
                    for (int l = 0; l < dim; l++) {
                        complex_num mul_res = mul_complex(output[i][l], matrices[k][l][j]);
                        temp[i][j] = add_complex(temp[i][j], mul_res);
                    }
                }
            }

            for (int i = 0; i < dim; i++) {
                for (int j = 0; j < dim; j++) {
                    output[i][j] = temp[i][j];
                }
            }
        }

        for (int i = 0; i < dim; i++) {
            free(temp[i]);
        }
        free(temp);
    }
    return output;
}

bool is_Binary(int ** binary_matrix,int rows,int cols){
    for (int i=0; i<rows; i++){
        for (int j=0; j<cols; j++){
            if (binary_matrix[i][j]!=0&&binary_matrix[i][j]!=1){
                return false;
            }
        }
    }
    return true;
}

int** manage_AND_OR_matrix(char* operation,int** binary_matrix1,int first_rows,int first_cols,int** binary_matrix2){
    int** output = (int**)malloc(first_rows * first_cols * sizeof(int));
    for (int i = 0; i < first_rows; i++) {
        output[i] = (int*)malloc(first_cols * sizeof(int));
    }
    if (strcmp(operation,"OR")==0){
        for (int i = 0; i < first_rows; i++) {
            for (int j = 0; j < first_cols; j++) {
                output[i][j] = binary_matrix1[i][j] || binary_matrix2[i][j];
            }
        }
    }
    else {
        for (int i = 0; i < first_rows; i++) {
            for (int j = 0; j < first_cols; j++) {
                output[i][j] = binary_matrix1[i][j] && binary_matrix2[i][j];
            }
        }
    }
    return output;
}
int** and_or_all_matrices(char* operation, int*** matrices, int dim, int num_of_matrices) {
    int** output = (int**)malloc(dim * sizeof(int*));
    for (int i = 0; i < dim; i++) {
        output[i] = (int*)malloc(dim * sizeof(int));
    }
    if (strcmp(operation, "AND") == 0) {
        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j] = 1;
                for (int k = 0; k < num_of_matrices; k++) {
                    output[i][j] &= matrices[k][i][j];
                }
            }
        }
    }if (strcmp(operation, "OR") == 0) {
        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                output[i][j] = 0;
                for (int k = 0; k < num_of_matrices; k++) {
                    output[i][j] |= matrices[k][i][j];
                }
            }
        }
    }
    return output;
}

void print_matrix_complex(complex_num** matrix, int rows, int cols) {
    if (matrix==NULL) return;
    printf("(%d,%d:", rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i != 0 || j != 0) {
                printf(",");
            }
            if (matrix[i][j].Imaginary_part==0&&matrix[i][j].Real_part==0){
                printf("0i");
                continue;
            }
            if (matrix[i][j].Real_part==0&&matrix[i][j].Imaginary_part!=0){
                printf("%di",matrix[i][j].Imaginary_part);
                continue;
            }
            if (matrix[i][j].Real_part!=0&&matrix[i][j].Imaginary_part<0){
                printf("%d%di",matrix[i][j].Real_part,matrix[i][j].Imaginary_part);
                continue;
            }
            printf("%d+%di", matrix[i][j].Real_part, matrix[i][j].Imaginary_part);
        }
    }
    printf(")\n");
}
void print_matrix_double(double** matrix, int rows, int cols) {
    if (matrix==NULL) return;
    printf("(%d,%d:", rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i != 0 || j != 0) {
                printf(",");
            }
            printf("%.1f", matrix[i][j]);
        }
    }
    printf(")\n");
}
void print_matrix_int(int** matrix, int rows, int cols) {
    printf("(%d,%d:", rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i != 0 || j != 0) {
                printf(",");
            }
            printf("%d", matrix[i][j]);
        }
    }
    printf(")\n");
}

void* thread_function_int(void* args){
    input_int inputInt=*(input_int*)args;
    if (strcmp(inputInt.operation,"ADD")==0|| strcmp(inputInt.operation,"MUL")==0){
        int** result = add_mul_int(inputInt.operation,inputInt.matrix1,inputInt.matrix2,inputInt.dim);
        pthread_exit((void **)result);
    }
    else {
        if (!is_Binary(inputInt.matrix1,inputInt.dim,inputInt.dim)||!is_Binary(inputInt.matrix2,inputInt.dim,inputInt.dim)){
            pthread_exit(NULL);
        }
        int** result = manage_AND_OR_matrix(inputInt.operation,inputInt.matrix1,inputInt.dim,inputInt.dim,inputInt.matrix2);
        pthread_exit((void **)result);
    }
}
void* thread_function_double(void* args){
    input_double inputDouble=*(input_double*)args;
    double** result = add_mul_double(inputDouble.operation,inputDouble.matrix1,inputDouble.matrix2,inputDouble.dim);
    pthread_exit((void **)result);
}
void* thread_function_complex(void* args){
    input_complex inputsComplex=*(input_complex *)args;
    complex_num ** result = add_mul_complex(inputsComplex.operation,inputsComplex.matrix1,inputsComplex.matrix2,inputsComplex.dim);
    pthread_exit((void **)result);
}

bool isPowerOfTwo(int n) {
    if (n <= 0) {
        return false;
    }
    return (n & (n - 1)) == 0;
}
bool is_operation(const char *input) {
    return strcmp(input, "ADD") == 0 || strcmp(input, "SUB") == 0 || strcmp(input, "MUL") == 0 ||
           strcmp(input, "AND") == 0 || strcmp(input, "OR") == 0 ||
           strcmp(input, "NOT") == 0 || strcmp(input, "TRANSPOSE") == 0;
}

void matrix_to_string_int(int** matrix, int dim, char* str) {
    char temp[MAX_LEN];
    sprintf(str, "(%d,%d:", dim, dim);
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            sprintf(temp, "%d", matrix[i][j]);
            strcat(str, temp);
            if (i != dim - 1 || j != dim - 1) {
                strcat(str, ",");
            }
        }
    }
    strcat(str, ")");
}
void convert_matrices_to_chars_int(int*** matrices, int num_of_matrices, int dim, char result[num_of_matrices][MAX_LEN]) {
    if (matrices==NULL) return;
    for (int i = 0; i < num_of_matrices; i++) {
        matrix_to_string_int(matrices[i], dim, result[i]);
    }
}
void free_matrix_int(int** matrix, int dim) {
    for (int i = 0; i < dim; ++i) {
        free(matrix[i]);
    }
    free(matrix);
}

void matrix_to_string_double(double** matrix, int dim, char* str) {
    char temp[MAX_LEN];
    sprintf(str, "(%d,%d:", dim, dim);
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            sprintf(temp, "%.2f", matrix[i][j]);
            strcat(str, temp);
            if (i != dim - 1 || j != dim - 1) {
                strcat(str, ",");
            }
        }
    }
    strcat(str, ")");
}
void convert_matrices_to_chars_double(double*** matrices, int num_of_matrices, int dim, char result[num_of_matrices][MAX_LEN]) {
    if (matrices == NULL) return;
    for (int i = 0; i < num_of_matrices; i++) {
        matrix_to_string_double(matrices[i], dim, result[i]);
    }
}
void free_matrix_double(double** matrix, int dim) {
    for (int i = 0; i < dim; ++i) {
        free(matrix[i]);
    }
    free(matrix);
}

void matrix_to_string_complex_num(complex_num** matrix, int dim, char* str) {
    char temp[MAX_LEN];
    sprintf(str, "(%d,%d:", dim, dim);
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            sprintf(temp, "%d%+di", matrix[i][j].Real_part, matrix[i][j].Imaginary_part);
            strcat(str, temp);
            if (i != dim - 1 || j != dim - 1) {
                strcat(str, ",");
            }
        }
    }
    strcat(str, ")");
}
void convert_matrices_to_chars_complex_num(complex_num*** matrices, int num_of_matrices, int dim, char result[num_of_matrices][MAX_LEN]) {
    if (matrices == NULL) return;
    for (int i = 0; i < num_of_matrices; i++) {
        matrix_to_string_complex_num(matrices[i], dim, result[i]);
    }
}
void free_matrix_complex_num(complex_num** matrix, int dim) {
    for (int i = 0; i < dim; ++i) {
        free(matrix[i]);
    }
    free(matrix);
}

int** recursive_for_power_of_2_matrices_int(int*** results, int num_of_matrices, char matrices[MAX_LEN][MAX_LEN], char operation[MAX_LEN], int dim) {
    if (num_of_matrices == 1&&results!=NULL) {
        return results[0];
    }

    input_int inputsInt[num_of_matrices / 2];
    pthread_t threads[num_of_matrices / 2];
    if (results==NULL){
        results=(int***) malloc(num_of_matrices/2 *dim *dim *sizeof (int));
    }

    int status = 0;
    for (int i = 0; i < num_of_matrices - 1; i += 2) {
        inputsInt[i / 2].dim = dim;
        fill_matrix_int(matrices[i], &inputsInt[i / 2].matrix1, dim, dim);
        fill_matrix_int(matrices[i + 1], &inputsInt[i / 2].matrix2, dim, dim);
        strncpy(inputsInt[i / 2].operation, operation, MAX_LEN);

        int st = pthread_create(&threads[i / 2], NULL, thread_function_int, &inputsInt[i / 2]);
        if (st != 0) {
            perror("pthread creating failed");
            exit(EXIT_FAILURE);
        }

        pthread_join(threads[i / 2], (void**)&results[i / 2]);
        if (results[i / 2] == NULL) {
            status = -1;
        }
    }

    if (status == -1) {
        fputs("ERR: one of the matrices is not binary\n", stderr);
        return NULL;
    }

    num_of_matrices /= 2;
    convert_matrices_to_chars_int(results, num_of_matrices, dim, matrices);
    int** re= recursive_for_power_of_2_matrices_int(results, num_of_matrices, matrices, operation, dim);
    return re;
}
double** recursive_for_power_of_2_matrices_double (double*** results, int num_of_matrices, char matrices[MAX_LEN][MAX_LEN], char operation[MAX_LEN], int dim){
    if (num_of_matrices == 1&&results!=NULL) {
        return results[0];
    }

    input_double inputsDouble[num_of_matrices / 2];
    pthread_t threads[num_of_matrices / 2];
    if (results==NULL){
        results=(double ***) malloc(num_of_matrices/2 *dim *dim *sizeof (double));
    }


    for (int i = 0; i < num_of_matrices - 1; i += 2) {
        inputsDouble[i / 2].dim = dim;
        fill_matrix_double(matrices[i], &inputsDouble[i / 2].matrix1, dim, dim);
        fill_matrix_double(matrices[i + 1], &inputsDouble[i / 2].matrix2, dim, dim);
        strncpy(inputsDouble[i / 2].operation, operation, MAX_LEN);

        int st = pthread_create(&threads[i / 2], NULL, thread_function_double, &inputsDouble[i / 2]);
        if (st != 0) {
            perror("pthread creating failed");
            exit(EXIT_FAILURE);
        }

        pthread_join(threads[i / 2], (void**)&results[i / 2]);
    }

    num_of_matrices /= 2;
    convert_matrices_to_chars_double(results, num_of_matrices, dim, matrices);
    double **re =recursive_for_power_of_2_matrices_double(results, num_of_matrices, matrices, operation, dim);
    return re;
}
complex_num** recursive_for_power_of_2_matrices_complex (complex_num *** results, int num_of_matrices, char matrices[MAX_LEN][MAX_LEN], char operation[MAX_LEN], int dim){
    if (num_of_matrices == 1&&results!=NULL) {
        return results[0];
    }

    input_complex inputsComplex[num_of_matrices / 2];
    pthread_t threads[num_of_matrices / 2];
    if (results==NULL){
        results=(complex_num ***) malloc(num_of_matrices/2 *dim *dim *sizeof (complex_num));
    }


    for (int i = 0; i < num_of_matrices - 1; i += 2) {
        inputsComplex[i / 2].dim = dim;
        fill_matrix_complex(matrices[i], &inputsComplex[i / 2].matrix1, dim, dim);
        fill_matrix_complex(matrices[i + 1], &inputsComplex[i / 2].matrix2, dim, dim);
        strncpy(inputsComplex[i / 2].operation, operation, MAX_LEN);

        int st = pthread_create(&threads[i / 2], NULL, thread_function_complex, &inputsComplex[i / 2]);
        if (st != 0) {
            perror("pthread creating failed");
            exit(EXIT_FAILURE);
        }

        pthread_join(threads[i / 2], (void**)&results[i / 2]);
    }

    num_of_matrices /= 2;
    convert_matrices_to_chars_complex_num(results, num_of_matrices, dim, matrices);
    complex_num ** re=recursive_for_power_of_2_matrices_complex(results, num_of_matrices, matrices, operation, dim);
    return re;
}

int main(void) {
    while (1) {
        char input[MAX_LEN];
        char matrices[MAX_LEN][MAX_LEN];
        char operation[MAX_LEN];
        int num_of_matrices=0;

        fgets(input, MAX_LEN, stdin);
        input[strcspn(input, "\n")] = '\0';
        int if_complex=0;
        int if_double=0;
        int if_int=0;
        if (is_complex(input)){
            if_complex=1;
        }
        if (is_double(input)){
            if_double=1;
        }
        if (!is_complex(input)&&!is_double(input)){
            if_int=1;
        }

        if (strcmp(input, "END") == 0) {
            break;
        }

        if (!is_operation(input)) {
            num_of_matrices++;
            strncpy(matrices[0], input, MAX_LEN);
        }

        while (fgets(input, MAX_LEN, stdin) != NULL) { // Read and write them to the shm matrices until detecting an operation
            input[strcspn(input, "\n")] = '\0';
            if (is_operation(input)) break;
            if (strcmp(input, "END") == 0) {
                break;
            }
            num_of_matrices++;
            strncpy(matrices[num_of_matrices-1], input, MAX_LEN);
        }

        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "END") == 0) {

            break;
        }
        strncpy(operation, input, MAX_LEN);
        int dim  =  matrices[0][1]-'0';

        if (strcmp(operation,"AND")==0|| strcmp(operation,"OR")==0){
            if (if_complex||if_double){
                fprintf(stderr,"at least one matrix is not binary\n");
                continue;
            }
        }

        if (isPowerOfTwo(num_of_matrices)){
            if (if_int) {
                int ** result= recursive_for_power_of_2_matrices_int(NULL,num_of_matrices,matrices,operation,dim);
                print_matrix_int(result,dim,dim);
                free_matrix_int(result,dim);
            }
            if (if_double) {
                double ** result=recursive_for_power_of_2_matrices_double(NULL,num_of_matrices,matrices,operation,dim);
                print_matrix_double(result,dim,dim);
                free_matrix_double(result,dim);
            }
            if (if_complex) {
                complex_num ** results=recursive_for_power_of_2_matrices_complex(NULL,num_of_matrices,matrices,operation,dim);
                print_matrix_complex(results,dim,dim);
                free_matrix_complex_num(results,dim);
            }
            continue;
        }

        input_int inputsInt[num_of_matrices/2];
        input_double inputsDouble[num_of_matrices/2];
        input_complex inputsComplex[num_of_matrices/2];
        pthread_t threads[num_of_matrices/2];


        if (if_int){
            int** results[num_of_matrices/2];
            int status=0;
            for (int i=0; i<num_of_matrices-1; i+=2){
                inputsInt[i/2].dim=dim;
                fill_matrix_int(matrices[i],&inputsInt[i/2].matrix1,dim,dim);
                fill_matrix_int(matrices[i+1],&inputsInt[i/2].matrix2,dim,dim);
                strncpy(inputsInt[i/2].operation, operation, MAX_LEN);
                int st=pthread_create(&threads[i/2],NULL,thread_function_int,&inputsInt[i/2]);
                if (st==-1){
                    perror("pthread creating failed");
                    exit(EXIT_FAILURE);
                }
                pthread_join(threads[i/2],(void**)&results[i/2]);
                if (results[i/2]==NULL){
                    status=-1;
                }
            }
            if (status == -1) {
                fputs("ERR: one of the matrices is not binary\n",stderr);
                continue;
            }
            if (strcmp(operation,"ADD")==0){
                if (num_of_matrices==2){
                    print_matrix_int(results[0],dim,dim);
                    for (int k=0; k<dim; k++){
                        free(results[0][k]);
                    }
                    free(results[0]);
                    continue;
                }
                int** output=add_mul_all_matrices_int(operation,results,dim,num_of_matrices/2);
                if (output==NULL){
                    exit(EXIT_FAILURE);
                }
                print_matrix_int(output,dim,dim);
                for (int j=0; j<num_of_matrices/2; j++){
                    for (int k=0; k<dim; k++){
                        free(results[j][k]);
                    }
                    free(results[j]);
                }
                for (int i=0; i<dim; i++){
                    free(output[i]);
                }
                free(output);
                continue;
            }
            if (strcmp(operation,"MUL")==0){
                if (num_of_matrices==2){
                    print_matrix_int(results[0],dim,dim);
                    for (int k=0; k<dim; k++){
                        free(results[0][k]);
                    }
                    free(results[0]);
                    continue;
                }
                char last_matrix[MAX_LEN];
                char new_matrices[(num_of_matrices/2)-1][MAX_LEN];
                char res[num_of_matrices/2][MAX_LEN];
                convert_matrices_to_chars_int(results,num_of_matrices/2,dim,res);
                strcpy(last_matrix,res[num_of_matrices/2-1]);
                for (int k=0; k<(num_of_matrices/2)-1; k++){
                    strcpy(new_matrices[k],res[k]);
                }
                int** output= recursive_for_power_of_2_matrices_int(NULL,(num_of_matrices/2)-1,new_matrices,operation,dim);
                if (output==NULL){
                    exit(EXIT_FAILURE);
                }
                int** lastMatrix;
                fill_matrix_int(last_matrix,&lastMatrix,dim,dim);
                int** last_output=add_mul_int(operation,output,lastMatrix,dim);
                print_matrix_int(last_output,dim,dim);
                for (int j=0; j<num_of_matrices/2; j++){
                    free_matrix_int(results[j],dim);
                }
                free_matrix_int(lastMatrix,dim);
                free_matrix_int(output,dim);
                free_matrix_int(last_output,dim);
                continue;
            }
            if (strcmp(operation,"AND")==0|| strcmp(operation,"OR")==0){
                if (num_of_matrices==2){
                    print_matrix_int(results[0],dim,dim);
                    for (int k=0; k<dim; k++){
                        free(results[0][k]);
                    }
                    free(results[0]);
                    continue;
                }
                int** output=and_or_all_matrices(operation,results,dim,num_of_matrices/2);
                if (output==NULL){
                    exit(EXIT_FAILURE);
                }
                print_matrix_int(output,dim,dim);
                for (int j=0; j<num_of_matrices/2; j++){
                    for (int k=0; k<dim; k++){
                        free(results[j][k]);
                    }
                    free(results[j]);
                }
                for (int i=0; i<dim; i++){
                    free(output[i]);
                }
                free(output);
                continue;
            }
        }
        if (if_double){
            if (strcmp(operation,"AND")==0|| strcmp(operation,"OR")==0){
                fputs("ERR: one of the matrices is not binary\n",stderr);
                continue;
            }
            double** results[num_of_matrices/2];
            for (int i=0; i<num_of_matrices-1; i+=2){
                inputsDouble[i/2].dim=dim;
                fill_matrix_double(matrices[i],&inputsDouble[i/2].matrix1,dim,dim);
                fill_matrix_double(matrices[i+1],&inputsDouble[i/2].matrix2,dim,dim);
                strncpy(inputsDouble[i/2].operation, operation, MAX_LEN);
                int st=pthread_create(&threads[i/2],NULL,thread_function_double,&inputsDouble[i/2]);
                if (st==-1){
                    perror("pthread creating failed");
                    exit(EXIT_FAILURE);
                }
                pthread_join(threads[i/2],(void**)&results[i/2]);
            }
            if (strcmp(operation,"ADD")==0){
                if (num_of_matrices==2){
                    print_matrix_double(results[0],dim,dim);
                    for (int k=0; k<dim; k++){
                        free(results[0][k]);
                    }
                    free(results[0]);
                    continue;
                }
                double** output=add_mul_all_matrices_double(operation,results,dim,num_of_matrices/2);
                if (output==NULL){
                    exit(EXIT_FAILURE);
                }
                print_matrix_double(output,dim,dim);
                for (int j=0; j<num_of_matrices/2; j++){
                    for (int k=0; k<dim; k++){
                        free(results[j][k]);
                    }
                    free(results[j]);
                }
                for (int i=0; i<dim; i++){
                    free(output[i]);
                }
                free(output);
                continue;
            }
            if (strcmp(operation,"MUL")==0){
                if (num_of_matrices==2){
                    print_matrix_double(results[0],dim,dim);
                    for (int k=0; k<dim; k++){
                        free(results[0][k]);
                    }
                    free(results[0]);
                    continue;
                }
                char last_matrix[MAX_LEN];
                char new_matrices[(num_of_matrices/2)-1][MAX_LEN];
                char res[num_of_matrices/2][MAX_LEN];
                convert_matrices_to_chars_double(results,num_of_matrices/2,dim,res);
                strcpy(last_matrix,res[num_of_matrices/2-1]);
                for (int k=0; k<(num_of_matrices/2)-1; k++){
                    strcpy(new_matrices[k],res[k]);
                }
                double** output= recursive_for_power_of_2_matrices_double(NULL,(num_of_matrices/2)-1,new_matrices,operation,dim);
                if (output==NULL){
                    exit(EXIT_FAILURE);
                }
                double** lastMatrix;
                fill_matrix_double(last_matrix,&lastMatrix,dim,dim);
                double** last_output=add_mul_double(operation,output,lastMatrix,dim);
                print_matrix_double(last_output,dim,dim);
                for (int j=0; j<num_of_matrices/2; j++){
                    free_matrix_double(results[j],dim);
                }
                free_matrix_double(lastMatrix,dim);
                free_matrix_double(output,dim);
                free_matrix_double(last_output,dim);
                continue;
            }
        }
        if (if_complex){
            if (strcmp(operation,"AND")==0|| strcmp(operation,"OR")==0){
                fputs("ERR: one of the matrices is not binary\n",stderr);
                continue;
            }
            complex_num ** results[num_of_matrices/2];
            for (int i=0; i<num_of_matrices-1; i+=2){
                inputsComplex[i/2].dim=dim;
                fill_matrix_complex(matrices[i],&inputsComplex[i/2].matrix1,dim,dim);
                fill_matrix_complex(matrices[i+1],&inputsComplex[i/2].matrix2,dim,dim);
                strncpy(inputsComplex[i/2].operation, operation, MAX_LEN);
                int st=pthread_create(&threads[i/2],NULL,thread_function_complex,&inputsComplex[i/2]);
                if (st==-1){
                    perror("pthread creating failed");
                    exit(EXIT_FAILURE);
                }
                pthread_join(threads[i/2],(void**)&results[i/2]);
            }
            if (strcmp(operation,"ADD")==0){
                if (num_of_matrices==2){
                    print_matrix_complex(results[0],dim,dim);
                    for (int k=0; k<dim; k++){
                        free(results[0][k]);
                    }
                    free(results[0]);
                    continue;
                }
                complex_num ** output=add_mul_all_matrices_complex(operation,results,dim,num_of_matrices/2);
                if (output==NULL){
                    continue;
                }
                print_matrix_complex(output,dim,dim);
                for (int j=0; j<num_of_matrices/2; j++){
                    for (int k=0; k<dim; k++){
                        free(results[j][k]);
                    }
                    free(results[j]);
                }
                for (int i=0; i<dim; i++){
                    free(output[i]);
                }
                free(output);
                continue;
            }
            if (strcmp(operation,"MUL")==0){
                if (num_of_matrices==2){
                    print_matrix_complex(results[0],dim,dim);
                    for (int k=0; k<dim; k++){
                        free(results[0][k]);
                    }
                    free(results[0]);
                    continue;
                }
                char last_matrix[MAX_LEN];
                char new_matrices[(num_of_matrices/2)-1][MAX_LEN];
                char res[num_of_matrices/2][MAX_LEN];
                convert_matrices_to_chars_complex_num(results,num_of_matrices/2,dim,res);
                strcpy(last_matrix,res[num_of_matrices/2-1]);
                for (int k=0; k<(num_of_matrices/2)-1; k++){
                    strcpy(new_matrices[k],res[k]);
                }
                complex_num ** output= recursive_for_power_of_2_matrices_complex(NULL,(num_of_matrices/2)-1,new_matrices,operation,dim);
                if (output==NULL){
                    exit(EXIT_FAILURE);
                }
                complex_num ** lastMatrix;
                fill_matrix_complex(last_matrix,&lastMatrix,dim,dim);
                complex_num ** last_output=add_mul_complex(operation,output,lastMatrix,dim);
                print_matrix_complex(last_output,dim,dim);
                for (int j=0; j<num_of_matrices/2; j++){
                    free_matrix_complex_num(results[j],dim);
                }
                free_matrix_complex_num(lastMatrix,dim);
                free_matrix_complex_num(output,dim);
                free_matrix_complex_num(last_output,dim);
                continue;
            }
        }
    }
}
