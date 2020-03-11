#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpi.h"

/*
 * Belal Hamdy Ezzat
 * 20170077
 * CS_1
 * Parallel Processing
 * Program to multiply two matrices using parallel processing concepts
 * -----------------------
 * First i get the transpose of matrix B to deal with it as rows not columns to easily send it as 1D array
 * To increase the performance I take it transposed from the user
 */
typedef enum {
    true, false
} bool;

void releaseArray(int **arr, int rows) {
    for (int x = 0; x < rows; x++)
        free(arr[x]);
    free(arr);
}

int **getMatrixFromUser(int rows, int columns, bool transpose) {
    int **arr = malloc(sizeof(int *) * rows);
    for (int i = 0; i < rows; ++i) {
        arr[i] = malloc(sizeof(int *) * columns);
    }

    printf("Enter the Data of Array\n");
    if (transpose == true)
        for (int i = 0; i < columns; ++i)
            for (int j = 0; j < rows; ++j)
                scanf("%d", &arr[j][i]);
    else
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < columns; ++j)
                scanf("%d", &arr[i][j]);

    return arr;

}

void printMatrix(int **arr, int rows, int columns) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            printf("%d ", arr[i][j]);
        }
        puts("");
    }
}

int multiplyTwoRows(const int *first, const int *second, int size) {
//    for (int i = 0; i < size; ++i) printf("%d ", first[i]);
//    puts("");
//    for (int i = 0; i < size; ++i) printf("%d ", second[i]);
//    puts("");
    int sum = first[0] * second[0];
    for (int i = 1; i < size; ++i) sum += first[i] * second[i];
    return sum;
}

/* the second matrix is transposed
 *  Multiplication: R1,C1 * R2,C2 ->  R1,C2 && C1 == R1
 */
int **matrixMultiplication(int **first, int first_rows, int **second, int second_columns, int mul_size) {
    int **arr = malloc(sizeof(int *) * first_rows);
    for (int i = 0; i < first_rows; ++i)
        arr[i] = malloc(sizeof(int *) * second_columns);

    for (int i = 0; i < first_rows; ++i)
        for (int j = 0; j < second_columns; ++j)
            arr[i][j] = multiplyTwoRows(first[i], second[j], mul_size);

    return arr;
}

int main(int argc, char *argv[]) {
    int my_rank;        /* rank of process	*/
    int p;            /* number of process	*/
    int source;        /* rank of sender	*/
    int dest;        /* rank of reciever	*/
    int tag = 0;        /* tag for messages	*/
    char message[100];    /* storage for message	*/
    MPI_Status status;    /* return status for 	*/
    /* recieve		*/

    /* Start up MPI */
    MPI_Init(&argc, &argv);

    /* Find out process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
/* Find out number of process */
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if (my_rank != 0) {
        int flag = 1;
        int mul_size = 0;
        MPI_Bcast(&mul_size, 1, MPI_INT, 0, MPI_COMM_WORLD);


        int *first_row = malloc(sizeof(int *) * mul_size);
        int *second_row = malloc(sizeof(int *) * mul_size);
        while (flag == 1) {
            MPI_Recv(&flag, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
            if(flag == 0){
                break;
            }
            MPI_Recv(first_row, mul_size, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
            MPI_Recv(second_row, mul_size, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);

            int res = multiplyTwoRows(first_row, second_row, mul_size);

            MPI_Send(&res, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
        }
    } else {
        int first_rows, first_columns;
        printf("Enter number rows of first array: ");
        scanf("%d", &first_rows);
        printf("Enter number of columns of first array: ");
        scanf("%d", &first_columns);

        if (first_rows <= 0 || first_columns <= 0) {
            printf("First array rows/columns can't be negative.\n");
            MPI_Finalize();
            return 0;
        }

        int **first_array = getMatrixFromUser(first_rows, first_columns, false);

        // Transposes the array
        int second_rows, second_columns;
        printf("Enter number rows of second array: ");
        scanf("%d", &second_rows);
        printf("Enter number of columns of second array: ");
        scanf("%d", &second_columns);

        if (second_rows <= 0 || second_columns <= 0) {
            printf("Second array rows/columns can't be negative.\n");
            releaseArray(first_array, first_rows);
            MPI_Finalize();
            return 0;
        }

        int **second_array = getMatrixFromUser(second_columns, second_rows, true);

        // second rows is second columns
        // second columns is second rows because of transpose
        // Multiplication condition has failed.
        if (first_columns != second_rows) {
            printf("Multiplication condition has failed..\n");
            releaseArray(first_array, first_rows);
            releaseArray(second_array, second_rows);
            MPI_Finalize();
            return 0;
        }
        int **ans;
        // if there are 2 nodes there is no need to use parallel programming because the second node will perform all work
        if (p <= 2) {
            printf("There is no sufficient nodes so it will be processed on master node.\n");
            ans = matrixMultiplication(first_array, first_rows, second_array, second_columns, first_columns);
        } else {
            int mul_size = first_columns;
            MPI_Bcast(&mul_size, 1, MPI_INT, 0, MPI_COMM_WORLD); // sends the size to all processors

            int one = 1;
            int zero = 0;
            int turn = 0;
            int slaves = p - 1;
            for (int i = 0; i < first_rows; ++i) {
                for (int j = 0; j < second_columns; ++j) {
                    ++turn;
                    MPI_Send(&one, 1, MPI_INT, turn, tag, MPI_COMM_WORLD);

                    MPI_Send(first_array[i], mul_size, MPI_INT, turn, tag, MPI_COMM_WORLD);
                    MPI_Send(second_array[j], mul_size, MPI_INT, turn, tag, MPI_COMM_WORLD);

                    turn %= slaves;
                }
            }

            for(turn = 1 ; turn<p ; ++turn) MPI_Send(&zero, 1, MPI_INT, turn, tag, MPI_COMM_WORLD);

            ans = malloc(sizeof(int *) * first_rows);
            for (int i = 0; i < first_rows; ++i)
                ans[i] = malloc(sizeof(int *) * second_columns);

            turn = 0;
            for (int i = 0; i < first_rows; ++i) {
                for (int j = 0; j < second_columns; ++j) {
                    ++turn;
                    MPI_Recv(&ans[i][j], mul_size, MPI_INT, turn, tag, MPI_COMM_WORLD, &status);
                    turn %= slaves;
                }
            }

        }
        printMatrix(ans, first_rows, second_columns);

        releaseArray(first_array, first_rows);
        releaseArray(second_array, second_rows);
        releaseArray(ans, first_rows);
    }
    /* shutdown MPI */
    MPI_Finalize();
    return 0;
}