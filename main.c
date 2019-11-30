#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "/usr/include/mpi/mpi.h"
#include <malloc.h>

typedef struct {
    int row,col,depth,numOfRowsForEachProcess;
}PGMFileParameters;

PGMFileParameters readPGM(){
    FILE *fd;
    FILE *fdd;

//Extract the file info from header
    char type;
    PGMFileParameters fileparams;
    fd=fopen("../barbara.pgm","rb");
    fscanf(fd,"%s",&type);
    int startWorking = 0;
    while(getc(fd) != '\n');
    while (startWorking == 0 && getc(fd) == '#')              /* skip comment lines */
    {
        while (getc(fd) != '\n');          /* skip to end of comment line */
        startWorking = 1;
    }
    fscanf(fd,"%d", &fileparams.row);
    fscanf(fd,"%d", &fileparams.col);
    fscanf(fd,"%d", &fileparams.depth);

//Malloc
    int** sudo=(int **)malloc(fileparams.row*sizeof(int *));
    if ( sudo != NULL){
        for (int k=0; k<fileparams.row ;k++){
            sudo[k] =(int*) calloc (fileparams.col,sizeof (int));
        }
    }

//Read the image data from the rest of the file
    for(int i=0;i<fileparams.row;i++){
        for(int j=0;j<fileparams.col;j++){
            fscanf(fd,"%d ", &sudo[i][j]);
        }
    }
    fclose(fd);


    printf("Header:\nfile type: %s\nrow: %d\ncol: %d\ndepht: %d\n",&type,fileparams.row,fileparams.col,fileparams.depth);

//Copy the matrix from memory to a new file
    fdd=fopen("../matrix.txt","wb");

    for(int i=0;i<fileparams.row;i++){
        for(int j=0;j<fileparams.col;j++){
            fprintf(fdd,"%d ",sudo[i][j]);
        }
        fprintf(fdd,"\n");
    }

    free(sudo);
    fclose(fdd);

    return fileparams;
}

void readMatrixValues(int** origMatrix, int row, int col){
    FILE *fd=fopen("../matrix.txt","rb");
    origMatrix[0][0]=0;
    for(int i=0;i<row;i++){
        for(int j=0;j<col;j++){
            int value = -1;
            fscanf(fd,"%d", &value);
            origMatrix[i][j] = value;
        }
    }
}

void makeFilter(int **origMatrix,int **newMatrix, PGMFileParameters fileParams){
    double newCellValue = 1,newCellValue2;
    float power = 0.1111111111;
    for(int currRow = 0; currRow<fileParams.row; currRow++){
        for (int currCol = 0; currCol < fileParams.col + 2; currCol++) {
            for (int rowTmp = currRow - 1; rowTmp < currRow + 2; rowTmp++) {
                if (rowTmp < 0 || rowTmp > fileParams.row - 1)
                    continue;
                for (int colTmp = currCol- 1; colTmp < currCol + 2; colTmp++) {
                    if (colTmp < 0 || colTmp > fileParams.col - 1)
                        continue;
                    int observedCellValue = origMatrix[rowTmp][colTmp];
                    newCellValue = newCellValue * observedCellValue;
                }
            }
            newCellValue = pow(newCellValue, power);
            int newCellValueInt = newCellValue;
            newMatrix[currRow][currCol] = newCellValueInt;
            newCellValue = 1;
        }
    }

    free (origMatrix);
}

void writeNewMatrixToFile(int **newMatrix, PGMFileParameters fileParams){
    FILE *fd=fopen("../newMatrix.txt","wb");

    for(int i=0;i<fileParams.row;i++){
        for(int j=0;j<fileParams.col;j++){
            fprintf(fd,"%d ",newMatrix[i][j]);
        }
        fprintf(fd,"\n");
    }

    free(newMatrix);
    fclose(fd);
}

int main(int argc, char **argv) {
    int mytid, nproc;
    int MASTER = 0;
    int tag = 99;
    MPI_Status status;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mytid);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    printf("MPI task ID = %d\n", mytid);
    if (mytid == 0) {//manager
        int **origMatrix, **newMatrix, numOfProcWaitingReceive = 0;
        if (nproc > 1){
            numOfProcWaitingReceive = nproc - 1;
    }
        PGMFileParameters fileParams = readPGM();
        fileParams.numOfRowsForEachProcess = (int)floor(fileParams.row / nproc);
        int numberOfLinesForManager =  fileParams.row % nproc;
        origMatrix = malloc(fileParams.row * sizeof *origMatrix);
        newMatrix = malloc(fileParams.row * sizeof *newMatrix);
        for (int i = 0; i < fileParams.row; i++) {
            origMatrix[i] = malloc(fileParams.col * sizeof *origMatrix[i]);
            newMatrix[i] = malloc(fileParams.col * sizeof *newMatrix[i]);
        }
        int myLongArray[fileParams.row * fileParams.col];
        int sizeForReceive = fileParams.numOfRowsForEachProcess * fileParams.col;

while (numOfProcWaitingReceive > 0){
    MPI_Recv(myLongArray[fileParams.col * fileParams.numOfRowsForEachProcess *(nproc - numOfProcWaitingReceive)], fileParams.col * fileParams.numOfRowsForEachProcess , MPI_INT, (nproc - numOfProcWaitingReceive), 1, MPI_COMM_WORLD, &status);
}

        readMatrixValues(origMatrix, fileParams.row, fileParams.col);
        makeFilter(origMatrix, newMatrix, fileParams);
        writeNewMatrixToFile(newMatrix, fileParams);
    }

    return 0;
}





//
//void func(int** origMatrix, int** array, PGMFileParameters file)
//{
//
//    for (int i=0; i<file.row; i++)
//    {
//        for (int j=0; j<file.col; j++)
//        {
//            array[i][j] = i*j;
//            origMatrix[i][j] = i*j;
//        }
//    }
//}
//
//void funcTmp(int **origMatrix, PGMFileParameters file){
//    origMatrix =  malloc(file.row * sizeof *origMatrix);
//    for (int i=0; i<file.row; i++)
//    {
//        origMatrix[i] = malloc(file.col * sizeof *origMatrix[i]);
//    }
//}
//
//int main()
//{
//    int rows = 10, cols =15, i;
//    PGMFileParameters file;
//    file.row = 10; file.col = 15;
//    int **origMatrix, **x;
//
//    /* obtain values for rows & cols */
//
//    /* allocate the array */
//    x = malloc(rows * sizeof *x);
//    for (i=0; i<rows; i++)
//    {
//        x[i] = malloc(cols * sizeof *x[i]);
//    }
//funcTmp(origMatrix,file);
//    /* use the array */
//
//    func(origMatrix, x, file);
//
//    /* deallocate the array */
//    for (i=0; i<rows; i++)
//    {
//        free(x[i]);
//    }
//    free(x);
//}

//////
//////struct Matrix{
//////    int matrix[3][3];
//////};
//////
//////struct Matrix createMatrix(){
//////    struct Matrix matrix = {
//////            {
//////                    {1, 2, 3},
//////                    {4, 5, 6},
//////                    {7, 8, 9}
//////            }
//////    };
//////    return matrix;
//////}
//////struct Matrix OrigMatrix = createMatrix();
//////struct Matrix newMatrix;
//////for (int i = 0; i<3; i++){
//////for(int j = 0; j<3; j++){
//////newMatrix.matrix[i][j] = newCellValue(i, j, OrigMatrix);
//////}
//////}
//////printMatrix(newMatrix);
//////
//////
//////
//////
//////void printMatrix(struct Matrix mat){
//////    for (int i = 0; i<3; i++){
//////        for(int j = 0; j<3; j++){
//////            printf("%d      ",mat.matrix[i][j]);
//////        }
//////        printf("\n");
//////    }
//////}
//////
//////int newCellValue(int col, int row, struct Matrix origMat){
//////    int newCellValue = 1;
//////    for(int colTmp = col -1; colTmp<col+2; colTmp++){
//////        if(colTmp < 0 || colTmp > 2)
//////            continue;
//////        for(int rowTmp = row -1; rowTmp<row+2; rowTmp++){
//////            if(rowTmp < 0 || rowTmp > 2)
//////                continue;
//////            int observedCellValue = origMat.matrix[colTmp][rowTmp];
//////            newCellValue = newCellValue * observedCellValue;
//////        }
//////    }
//////    return newCellValue;
//////}