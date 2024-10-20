#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define MAX_LEN 128
typedef struct {
    int Imaginary_part;
    int Real_part;
}complex_num;

complex_num add_complex(complex_num num1,complex_num num2){
    complex_num res;
    res.Imaginary_part=num1.Imaginary_part+num2.Imaginary_part;
    res.Real_part=num1.Real_part+num2.Real_part;
    return res;
}
complex_num sub_complex(complex_num num1,complex_num num2){
    complex_num res;
    res.Imaginary_part=num1.Imaginary_part-num2.Imaginary_part;
    res.Real_part=num1.Real_part-num2.Real_part;
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

int check_matrix(char* matrix,int rows,int cols){
    const char* values_start = strchr(matrix, ':') + 1;
    char* values_copy = strdup(values_start);  // Make a modifiable copy of the values string
    char* token = strtok(values_copy, ",");
    int k = 0;
    int count=0;
    while (token != NULL) {
        count++;
        if (count>(rows*cols)){
            free(values_copy);
            return -1;
        }
        k++;
        token = strtok(NULL, ",");
    }
    if (count!=(rows*cols)){
        free(values_copy);
        return -1;
    }
    free(values_copy);
    return 0;
}

int is_complex(const char* str) {
    return strchr(str, 'i') != NULL;
}
int is_double(const char* str) {
    return strchr(str, '.') != NULL;
}
int is_integer(const char* str) {
    while (*str) {
        if (!isdigit(*str) && *str != '-' && *str != '+') {
            return 0;
        }
        str++;
    }
    return 1;
}

double rou (double x){
    x= round(x*10) /10;
    return x;
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
        (*matrix)[k / cols][k % cols] = value;
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

complex_num** add_sub_mul_complex(char* operation, complex_num** first_matrix, complex_num** second_matrix, int first_rows, int first_cols) {
    complex_num** output = (complex_num**)malloc(first_rows * first_rows * sizeof(complex_num));
    if (strcmp(operation, "SUB") == 0) {
        for (int i = 0; i < first_rows; i++) {
            output[i] = (complex_num*)malloc(first_cols * sizeof(complex_num));
            for (int j = 0; j < first_cols; j++) {
                output[i][j] = sub_complex(first_matrix[i][j], second_matrix[i][j]);
            }
        }
        return output;
    }
    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < first_rows; i++) {
            output[i] = (complex_num*)malloc(first_rows * sizeof(complex_num));
            for (int j = 0; j < first_rows; j++) {
                output[i][j] = add_complex(first_matrix[i][j], second_matrix[i][j]);
            }
        }
        return output;
    }  if (strcmp(operation, "MUL")==0) {
        for (int i = 0; i < first_rows; i++) {
            output[i] = (complex_num*)malloc(first_rows * sizeof(complex_num));
            for (int j = 0; j < first_rows; j++) {
                output[i][j].Imaginary_part = 0;
                output[i][j].Real_part = 0;
            }
        }
        for (int i = 0; i < first_rows; i++) {
            for (int j = 0; j < first_rows; j++) {
                for (int k = 0; k < first_cols; k++) {
                    complex_num product = mul_complex(first_matrix[i][k], second_matrix[k][j]);
                    output[i][j] = add_complex(output[i][j], product);
                }
            }
        }

        return output;
    }

    return NULL;
}
double** add_sub_mul_double(char* operation, double** first_matrix, double** second_matrix, int first_rows, int first_cols) {
    double ** output = (double **)malloc(first_cols * first_rows*  sizeof(double ));
    if (strcmp(operation, "SUB") == 0) {
        for (int i = 0; i < first_rows; i++) {
            output[i] = (double*)malloc(first_cols * sizeof(double));
            for (int j = 0; j < first_cols; j++) {
                output[i][j] = first_matrix[i][j] - second_matrix[i][j];
            }
        }
    }
    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < first_rows; i++) {
            output[i] = (double*)malloc(first_cols * sizeof(double));
            for (int j = 0; j < first_cols; j++) {
                output[i][j] = first_matrix[i][j] + second_matrix[i][j];
            }
        }
        return output;

    }
    if (strcmp(operation, "MUL")==0) {

        for (int i = 0; i < first_rows; i++) {
            output[i] = (double *)malloc(first_cols * sizeof(double ));
        }

        for (int i = 0; i < first_cols; i++) {
            for (int j = 0; j < first_rows; j++) {
                output[i][j] = 0;
                for (int k = 0; k < first_cols; k++) {
                    output[i][j] = rou(output[i][j]) + rou(first_matrix[i][k] * second_matrix[k][j]);
                }
            }
        }

        return output;
    }
    return NULL;
}
int** add_sub_mul_int(char* operation, int** first_matrix, int** second_matrix, int first_rows, int first_cols) {
    int** output = (int**)malloc(first_rows *  first_cols * sizeof(int));

    if (strcmp(operation, "SUB") == 0) {
        for (int i = 0; i < first_rows; i++) {
            output[i] = (int*)malloc(first_cols * sizeof(int));
            for (int j = 0; j < first_cols; j++) {
                output[i][j] = first_matrix[i][j] - second_matrix[i][j];
            }
        }
        return output;

    } if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < first_rows; i++) {
            output[i] = (int*)malloc(first_rows * sizeof(int));
            for (int j = 0; j < first_rows; j++) {
                output[i][j] = first_matrix[i][j] + second_matrix[i][j];
            }
        }
        return output;

    }  if (strcmp(operation, "MUL")==0){
        for (int i = 0; i < first_rows; i++) {
            output[i] = (int*)malloc(first_rows * sizeof(int));
        }

        for (int i = 0; i < first_rows; i++) {
            for (int j = 0; j < first_cols; j++) {
                output[i][j] = 0;
                for (int k = 0; k < first_rows; k++) {
                    output[i][j] += first_matrix[i][k] * second_matrix[k][j];
                }
            }
        }

        return output;
    }
    return NULL;
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

void NOT_matrix(int*** binary_matrix,int rows,int cols){
    for (int i=0; i<rows; i++){
        for (int j=0; j<cols; j++){
            if ((*binary_matrix)[i][j]==1) (*binary_matrix)[i][j]=0;
            else (*binary_matrix)[i][j]=1;
        }
    }
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

complex_num** TRANSPOSE_matrix_complex(complex_num ** matrix,int rows,int cols){
    complex_num** transposed=(complex_num**) malloc(cols * sizeof(complex_num*));
    for (int i=0; i<cols; i++){
        transposed[i]=(complex_num*) malloc(rows*sizeof (complex_num));
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            transposed[j][i] = (matrix)[i][j];
        }
    }
    return transposed;
}
double** TRANSPOSE_matrix_double(double ** matrix,int rows,int cols){
    double ** transposed=(double **) malloc(cols * sizeof(double *));
    for (int i=0; i<cols; i++){
        transposed[i]=(double *) malloc(rows*sizeof (double ));
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            transposed[j][i] = (matrix)[i][j];
        }
    }
    return transposed;
}
int ** TRANSPOSE_matrix_int(int ** matrix,int rows,int cols){
    int** transposed=(int**) malloc(cols * sizeof(int*));
    for (int i=0; i<cols; i++){
        transposed[i]=(int *) malloc(rows*sizeof (int ));
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            transposed[j][i] = (matrix)[i][j];
        }
    }
    return transposed;
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
                printf("0");
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

int main() {
    char first[MAX_LEN];
    char second[MAX_LEN];
    char operation[MAX_LEN];

    while (1) {
        fgets(first, MAX_LEN, stdin);
        first[strcspn(first, "\n")] = '\0';
        if (strcmp(first,"END")==0){
            break;
        }

        fgets(second, MAX_LEN, stdin);
        second[strcspn(second, "\n")] = '\0';
        if (strcmp(second,"END")==0){
            break;
        }


        int first_rows = first[1] - '0';
        int first_cols = first[3] - '0';

        if (strcmp(second,"NOT")==0){
            if (is_complex(first)|| is_double(first)){
                fputs ("ERR: the matrix is not binary\n",stderr);
                continue;
            }
            int** binary_matrix;
            fill_matrix_int(first,&binary_matrix,first_rows,first_cols);
            if (!is_Binary(binary_matrix,first_rows,first_cols)){
                fprintf(stderr,"ERR: the matrix is not binary\n");
                free(binary_matrix);
            }
            NOT_matrix(&binary_matrix,first_rows,first_cols);
            print_matrix_int(binary_matrix,first_rows,first_cols);
            free(binary_matrix);
            continue;
        }

        if(strcmp(second,"TRANSPOSE")==0){
            if (strchr(first,'i')){
                complex_num** com_matrix;
                fill_matrix_complex(first,&com_matrix,first_rows,first_cols);
                complex_num** output=TRANSPOSE_matrix_complex(com_matrix,first_rows,first_cols);
                print_matrix_complex(output,first_cols,first_rows);
                free(com_matrix);
                if (output!=NULL){
                    for( int i =0; i<first_cols; i++){
                        free(output[i]);
                    }
                    free(output);
                }
            }
            else if (strchr(first,'.')){
                double** do_matrix;
                fill_matrix_double(first,&do_matrix,first_rows,first_cols);
                double ** output=TRANSPOSE_matrix_double(do_matrix,first_rows,first_cols);
                print_matrix_double(output,first_cols,first_rows);
                free(do_matrix);
                if (output!=NULL){
                    for( int i =0; i<first_cols; i++){
                        free(output[i]);
                    }
                    free(output);
                }
            }
            else {
                int** int_matrix;
                fill_matrix_int(first,&int_matrix,first_rows,first_cols);
                int** output=TRANSPOSE_matrix_int(int_matrix,first_rows,first_cols);
                print_matrix_int(output,first_cols,first_rows);
                free(int_matrix);
                if (output!=NULL){
                    for( int i =0; i<first_cols; i++){
                        free(output[i]);
                    }
                    free(output);
                }
            }
            continue;
        }

        int sec_rows = second[1] - '0';
        int sec_cols = second[3] - '0';

        fgets(operation, MAX_LEN, stdin);
        operation[strcspn(operation, "\n")] = '\0';

        if (strcmp(operation,"END")==0){
            break;
        }

        if (strcmp(operation,"AND")==0|| strcmp(operation,"OR")==0){
            if (is_complex(first)|| is_double(first)|| is_complex(second)|| is_double(second)){
                fputs ("ERR: at least one of matrices is not binary\n",stderr);
                continue;
            }
            int** binary_matrix1;
            int** binary_matrix2;
            fill_matrix_int(first,&binary_matrix1,first_rows,first_cols);
            if (!is_Binary(binary_matrix1,first_rows,first_cols)){
                fprintf(stderr,"ERR: the matrix is not binary\n");
                free(binary_matrix1);
                continue;
            }

            fill_matrix_int(second,&binary_matrix2,sec_rows,sec_cols);
            if (!is_Binary(binary_matrix2,sec_rows,sec_cols)){
                fprintf(stderr,"ERR: the matrix is not binary\n");
                free(binary_matrix2);
                continue;
            }

            int ** output=manage_AND_OR_matrix(operation,binary_matrix1,first_rows,first_cols,binary_matrix2);
            if (output==NULL)continue;
            print_matrix_int(output,first_rows,first_cols);
            free(binary_matrix1);
            free(binary_matrix2);
            for (int i=0; i<first_rows; i++){
                free(output[i]);
            }
            free(output);
            continue;
        }

        if (strcmp(operation,"ADD")==0|| strcmp(operation,"SUB")==0|| strcmp(operation,"MUL")==0) {
            if (strchr(first,'i')|| strchr(second,'i')) {
                complex_num **first_matrix;
                complex_num **second_matrix;
                fill_matrix_complex(first, &first_matrix, first_rows, first_cols);
                fill_matrix_complex(second, &second_matrix, sec_rows, sec_cols);
                complex_num **output = add_sub_mul_complex(operation, first_matrix, second_matrix, first_rows, first_cols);
                if (strcmp(operation,"MUL")==0){
                    print_matrix_complex(output, first_rows, sec_cols);
                }
                else print_matrix_complex(output, first_rows, first_cols);
                free(first_matrix);
                free(second_matrix);
                for (int i = 0; i < first_rows; i++) {
                    free(output[i]);
                }
                free(output);
            } else if (strchr(first,'.')|| strchr(second,'.')) {
                double **first_matrix;
                double **second_matrix;
                fill_matrix_double(first, &first_matrix, first_rows, first_cols);
                fill_matrix_double(second, &second_matrix, sec_rows, sec_cols);
                double **output = add_sub_mul_double(operation, first_matrix, second_matrix, first_rows, first_cols);
                if (strcmp(operation,"MUL")==0){
                    print_matrix_double(output, first_rows, sec_cols);
                }
                else print_matrix_double(output, first_rows, first_cols);
                free(first_matrix);
                free(second_matrix);
                for (int i = 0; i < first_rows; i++) {
                    free(output[i]);
                }
                free(output);
            } else{
                int **first_matrix;
                int **second_matrix;
                fill_matrix_int(first, &first_matrix, first_rows, first_cols);
                fill_matrix_int(second, &second_matrix, sec_rows, sec_cols);
                int **output = add_sub_mul_int(operation, first_matrix, second_matrix, first_rows, first_cols);
                if (output==NULL) continue;
                if (strcmp(operation,"MUL")==0){
                    print_matrix_int(output, first_rows, sec_cols);
                }
                else print_matrix_int(output, first_rows, first_cols);
                free(first_matrix);
                free(second_matrix);
                for (int i = 0; i < first_rows; i++) {
                    free(output[i]);
                }
                free(output);
            }
        }
    }
}

