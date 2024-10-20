#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <math.h>

#define MAX_LEN 128
#define SHM_SIZE 2048

typedef struct {
    int Imaginary_part;
    int Real_part;
} complex_num;

typedef struct {
    sem_t sem_writer;
    sem_t sem_reader;
    pthread_mutex_t mutex;
    char operation[MAX_LEN];
    int num_of_matrices;
    char matrices[MAX_LEN][MAX_LEN];
    int num_of_operations;
} shared_data;

int create_shared_memory() {
    key_t SHM_KEY = ftok("/tmp", 'A');
    if (SHM_KEY == -1) {
        perror("ftok() failed");
        exit(EXIT_FAILURE);
    }

    int shm_id = shmget(SHM_KEY, SHM_SIZE, 0600);
    if (shm_id < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shm_id;
}

void *attach_shared_memory(int shm_id) {
    void *shm_addr = shmat(shm_id, NULL, 0);
    if (shm_addr == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return shm_addr;
}
double rou (double x){
    x= round(x*10) /10;
    return x;
}
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

complex_num** add_sub_mul_complex(char* operation, complex_num** first_matrix, complex_num** second_matrix, int dim) {
    complex_num** output = (complex_num**)malloc(dim * dim * sizeof(complex_num));

    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < dim; i++) {
            output[i] = (complex_num*)malloc(dim * sizeof(complex_num));
            for (int j = 0; j < dim; j++) {
                output[i][j] = add_complex(first_matrix[i][j], second_matrix[i][j]);
            }
        }
        return output;
    }  if (strcmp(operation, "SUB") == 0) {

        for (int i = 0; i < dim; i++) {
            output[i] = (complex_num*)malloc(dim * sizeof(complex_num));
            for (int j = 0; j < dim; j++) {
                output[i][j] = sub_complex(first_matrix[i][j], second_matrix[i][j]);
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
double** add_sub_mul_double(char* operation, double** first_matrix, double** second_matrix, int dim) {
    double ** output = (double **)malloc(dim * dim*  sizeof(double ));
    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < dim; i++) {
            output[i] = (double*)malloc(dim * sizeof(double));
            for (int j = 0; j < dim; j++) {
                output[i][j] = first_matrix[i][j] + second_matrix[i][j];
            }
        }
        return output;

    }  if (strcmp(operation, "SUB") == 0) {
        for (int i = 0; i < dim; i++) {
            output[i] = (double*)malloc(dim * sizeof(double));
            for (int j = 0; j < dim; j++) {
                output[i][j] = first_matrix[i][j] - second_matrix[i][j];
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
int** add_sub_mul_int(char* operation, int** first_matrix, int** second_matrix, int dim) {
    int** output = (int**)malloc(dim *  dim * sizeof(int));
    if (strcmp(operation, "ADD") == 0) {
        for (int i = 0; i < dim; i++) {
            output[i] = (int*)malloc(dim * sizeof(int));
            for (int j = 0; j < dim; j++) {
                output[i][j] = first_matrix[i][j] + second_matrix[i][j];
            }
        }
        return output;

    }  if (strcmp(operation, "SUB") == 0) {
        for (int i = 0; i < dim; i++) {
            output[i] = (int*)malloc(dim * sizeof(int));
            for (int j = 0; j < dim; j++) {
                output[i][j] = first_matrix[i][j] - second_matrix[i][j];
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
            transposed[j][i] = matrix[i][j];
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
    int shm_id = create_shared_memory();
    shared_data *shared_data = attach_shared_memory(shm_id);

    while (shared_data->num_of_operations!=0) {
        if (strcmp(shared_data->operation,"END")==0) break;
        sem_wait(&shared_data->sem_reader);
        pthread_mutex_lock(&shared_data->mutex);
        if (shared_data->matrices[0]==NULL){
            break;
        }
        int if_complex=0;
        int if_double=0;
        int if_int=0;
        if (is_complex(shared_data->matrices[0])){
            if_complex=1;
        }
        if (is_double(shared_data->matrices[0])){
            if_double=1;
        }
        if (!is_complex(shared_data->matrices[0])&&!is_double(shared_data->matrices[0])){
            if_int=1;
        }



        int dim1 = shared_data->matrices[0][1] - '0';
        int dim2 = shared_data->matrices[1][1] - '0';

        if (strcmp(shared_data->operation,"NOT")==0){
            if (if_double||if_complex){
                fprintf(stderr,"ERR: matrix is not binary\n");
                shmctl(shm_id,IPC_RMID,NULL);
                pthread_mutex_unlock(&shared_data->mutex);
                sem_post(&shared_data->sem_writer);
                continue;
            }
            int** matrix;
            fill_matrix_int(shared_data->matrices[0],&matrix,dim1,dim1);
            if (!is_Binary(matrix,dim1,dim1)){
                fprintf(stderr,"ERR: MATRIX IS NOT BINARY\n");
                free(matrix);
                shmctl(shm_id,IPC_RMID,NULL);
                pthread_mutex_unlock(&shared_data->mutex);
                sem_post(&shared_data->sem_writer);
                continue;
            }
            NOT_matrix(&matrix,dim1,dim1);
            print_matrix_int(matrix,dim1,dim1);
            shared_data->num_of_operations--;
            free(matrix);
        }

        else if (strcmp(shared_data->operation,"TRANSPOSE")==0) {
            shared_data->num_of_operations--;
            if (if_double==1) {
                double **matrix;
                fill_matrix_double(shared_data->matrices[0], &matrix, dim1, dim1);
                double **trans_matrix = TRANSPOSE_matrix_double(matrix, dim1, dim1);
                print_matrix_double(trans_matrix, dim1, dim1);
                if (trans_matrix!=NULL){
                    for (int i = 0; i < dim1; i++) {
                        free(trans_matrix[i]);
                    }
                    free(trans_matrix);
                }

            } if (if_complex==1) {
                complex_num **matrix;
                fill_matrix_complex(shared_data->matrices[0], &matrix, dim1, dim1);
                complex_num **trans_matrix = TRANSPOSE_matrix_complex(matrix, dim1, dim1);
                print_matrix_complex(trans_matrix, dim1, dim1);
                if (trans_matrix!=NULL){
                    for (int i = 0; i < dim1; i++) {
                        free(trans_matrix[i]);
                    }
                    free(trans_matrix);
                }

            } if (if_int==1){
                int **matrix;
                fill_matrix_int(shared_data->matrices[0], &matrix, dim1, dim1);
                int **trans_matrix = TRANSPOSE_matrix_int(matrix, dim1, dim1);
                print_matrix_int(trans_matrix, dim1, dim1);
                if (trans_matrix!=NULL){
                    for (int i = 0; i < dim1; i++) {
                        free(trans_matrix[i]);
                    }
                    free(trans_matrix);
                }
            }
        }

        else if (strcmp(shared_data->operation,"ADD")==0||strcmp(shared_data->operation,"SUB")==0||strcmp(shared_data->operation,"MUL")==0){
            if (dim1!=dim2){
                fprintf(stderr,"ERR: MATRICES ARE NOT MATCHED\n");
                shmctl(shm_id,IPC_RMID,NULL);
                pthread_mutex_unlock(&shared_data->mutex);
                sem_post(&shared_data->sem_writer);
                continue;
            }
            shared_data->num_of_operations--;
            if (if_int){
                int** matrix1;
                int** matrix2;
                fill_matrix_int(shared_data->matrices[0],&matrix1,dim1,dim1);
                fill_matrix_int(shared_data->matrices[1],&matrix2,dim2,dim2);
                int** result= add_sub_mul_int(shared_data->operation,matrix1,matrix2,dim1);
                print_matrix_int(result,dim1,dim1);
                for (int i=0 ; i<dim1; i++){
                    free(result[i]);
                }
                free(result);
                free(matrix1);
                free(matrix2);
            }
            if (if_double){
                double ** matrix1;
                double ** matrix2;
                fill_matrix_double(shared_data->matrices[0],&matrix1,dim1,dim1);
                fill_matrix_double(shared_data->matrices[1],&matrix2,dim2,dim2);
                double ** result = add_sub_mul_double(shared_data->operation,matrix1,matrix2,dim1);
                print_matrix_double(result,dim1,dim1);
                for (int i=0 ; i<dim1; i++){
                    free(result[i]);
                }
                free(result);
                free(matrix1);
                free(matrix2);
            }
            if (if_complex){
                complex_num ** matrix1;
                complex_num ** matrix2;
                fill_matrix_complex(shared_data->matrices[0],&matrix1,dim1,dim1);
                fill_matrix_complex(shared_data->matrices[1],&matrix2,dim2,dim2);
                complex_num ** result= add_sub_mul_complex(shared_data->operation,matrix1,matrix2,dim1);
                print_matrix_complex(result,dim1,dim1);
                for (int i=0 ; i<dim1; i++){
                    free(result[i]);
                }
                free(result);
                free(matrix1);
                free(matrix2);
            }
        }

        else if (strcmp(shared_data->operation,"AND")==0||strcmp(shared_data->operation,"OR")==0){
            if (if_double||if_complex){
                fprintf(stderr,"ERR: one of matrices is not binary\n");
                shmctl(shm_id,IPC_RMID,NULL);
                pthread_mutex_unlock(&shared_data->mutex);
                sem_post(&shared_data->sem_writer);
                continue;
            }
            if (dim1!=dim2){
                fprintf(stderr,"ERR: MATRICES ARE NOT MATCHED\n");
                shmctl(shm_id,IPC_RMID,NULL);
                pthread_mutex_unlock(&shared_data->mutex);
                sem_post(&shared_data->sem_writer);
                continue;
            }
            shared_data->num_of_operations--;
            if (if_int){
                int** binary_matrix1;
                int** binary_matrix2;
                fill_matrix_int(shared_data->matrices[0],&binary_matrix1,dim1,dim1);
                if (!is_Binary(binary_matrix1,dim1,dim1)){
                    fprintf(stderr,"ERR: ON OF MATRICES IS NOT BINARY\n");
                    free(binary_matrix1);
                    shmctl(shm_id,IPC_RMID,NULL);
                    pthread_mutex_unlock(&shared_data->mutex);
                    sem_post(&shared_data->sem_writer);
                    continue;
                }

                fill_matrix_int(shared_data->matrices[1],&binary_matrix2,dim2,dim2);
                if (!is_Binary(binary_matrix2,dim2,dim2)){
                    fprintf(stderr,"ERR: ON OF MATRICES IS NOT BINARY\n");
                    free(binary_matrix2);
                    shmctl(shm_id,IPC_RMID,NULL);
                    pthread_mutex_unlock(&shared_data->mutex);
                    sem_post(&shared_data->sem_writer);
                    continue;
                }

                int ** output=manage_AND_OR_matrix(shared_data->operation,binary_matrix1,dim1,dim1,binary_matrix2);
                if (output==NULL)continue;
                print_matrix_int(output,dim1,dim1);
                for (int i=0 ; i<dim1; i++){
                    free(output[i]);
                }
                free(output);
                free(binary_matrix1);
                free(binary_matrix2);
            }
        }
        shmctl(shm_id,IPC_RMID,NULL);
        pthread_mutex_unlock(&shared_data->mutex);
        sem_post(&shared_data->sem_writer);
    }
}
