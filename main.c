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

void makeFilter(int mytid, int fileRows, int fileCols, int numOfRows, int** filter, int** origMatrix){
    int startRow = numOfRows * (mytid-1);
    double newCellValue = 1,newCellValue2;
    float power = 0.1111111111;
    int filterIdx = 0 ;
    for(int currRow = startRow; currRow<fileRows; currRow++){
        for (int currCol = 0; currCol < fileCols + 2; currCol++) {
            for (int rowTmp = currRow - 1; rowTmp < currRow + 2; rowTmp++) {
                if (rowTmp < 0 || rowTmp > fileRows - 1)
                    continue;
                for (int colTmp = currCol- 1; colTmp < currCol + 2; colTmp++) {
                    if (colTmp < 0 || colTmp > fileCols - 1)
                        continue;
                    int observedCellValue = origMatrix[rowTmp][colTmp];
                    newCellValue = newCellValue * observedCellValue;
                }
            }
            newCellValue = pow(newCellValue, power);
            int newCellValueInt = newCellValue;
            filter[filterIdx] = newCellValueInt;
            filterIdx++;
            newCellValue = 1;
        }
    }
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
    int tag = 1;
    int **origMatrix;
    MPI_Status status;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mytid);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    printf("MPI task ID = %d\n", mytid);
    if (mytid == 0) {//manager
         int **newMatrix, numOfProcWaitingReceive = 0;
        if (nproc > 1){
            numOfProcWaitingReceive = nproc - 1;
        }
        PGMFileParameters fileParams = readPGM();

        origMatrix = malloc(fileParams.row * sizeof *origMatrix);
        newMatrix = malloc(fileParams.row * sizeof *newMatrix);
        for (int i = 0; i < fileParams.row; i++) {
            origMatrix[i] = malloc(fileParams.col * sizeof *origMatrix[i]);
            newMatrix[i] = malloc(fileParams.col * sizeof *newMatrix[i]);
        }
        fileParams.numOfRowsForEachProcess = (int)floor(fileParams.row / (nproc - 1));
        int numberOfLinesForManager =  fileParams.row % nproc;
        int myLongArray[fileParams.row * fileParams.col];
        int sizeForReceive = fileParams.numOfRowsForEachProcess * fileParams.col;
        int fileParamsFOrSend[3] = {fileParams.numOfRowsForEachProcess,fileParams.row,fileParams.col};

        readMatrixValues(origMatrix, fileParams.row, fileParams.col);



        MPI_Bcast(fileParamsFOrSend, 3, MPI_INT, 0, MPI_COMM_WORLD);

        while (numOfProcWaitingReceive > 0){
            MPI_Recv(myLongArray[fileParams.col * fileParams.numOfRowsForEachProcess *(nproc - numOfProcWaitingReceive)], sizeForReceive , MPI_INT, (nproc - numOfProcWaitingReceive), 1, MPI_COMM_WORLD, &status);
            numOfProcWaitingReceive = numOfProcWaitingReceive - 1;
        }

        FILE *fd = fopen("../newMatrix2.txt", "wb");

        for (int i = 0; i < (fileParams.row * fileParams.col); i++) {
            fprintf(fd, "%d ", myLongArray[i]);
            if (i > 0 && i % fileParams.col == 0)
                fprintf(fd, "\n");
        }
            fprintf(fd, "\n");

      //  if (numberOfLinesForManager > 0)
            //makeFilter()

        free(newMatrix);
        fclose(fd);
    }
    else{//slave
        int err = MPI_Recv(
                 buff, //void *buf
                 count ,//int count
                 MPI_INT,//MPI_Datatype datatype
                 source,//int source,
                 1,// int tag
                 MPI_COMM_WORLD, //MPI_Comm comm
                 &status); //MPI_Status *status
        if(err != MPI_SUCCESS){
            printf("MPI_Recv Error %d, process %d exit.",err,mytid);
            return err;
        }
        int numOfRows = buff[0];
        int fileRows = buff[1];
        int fileCols = buff[2];
        int filter[numOfRows*col];
        makeFilter(mytid, fileRows, fileCols, numOfRows, filter, origMatrix);
        err = MPI_Send(
                       filter, //void *buf
                       numOfRows*col,//int count
                       MPI_INT,//MPI_Datatype datatype
                       0, // int dest
                       1,// int tag
                       MPI_COMM_WORLD);//MPI_Comm comm
        if(err != MPI_SUCCESS){
            printf("MPI_Send Error %d, process %d exit.",err,mytid);
            return err;
        }
    }

return 0;
}