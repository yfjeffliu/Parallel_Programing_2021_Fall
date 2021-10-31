//#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"
#include <mpi.h>
using namespace std;

//定義平滑運算的次數
#define NSmooth 1000

/*********************************************************/
/*變數宣告：                                             */
/*  bmpHeader    ： BMP檔的標頭                          */
/*  bmpInfo      ： BMP檔的資訊                          */
/*  **BMPSaveData： 儲存要被寫入的像素資料               */
/*  **BMPData    ： 暫時儲存要被寫入的像素資料           */
/*********************************************************/
BMPHEADER bmpHeader;
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;
RGBTRIPLE **BMPData = NULL;

/*********************************************************/
/*函數宣告：                                             */
/*  readBMP    ： 讀取圖檔，並把像素資料儲存在BMPSaveData*/
/*  saveBMP    ： 寫入圖檔，並把像素資料BMPSaveData寫入  */
/*  swap       ： 交換二個指標                           */
/*  **alloc_memory： 動態分配一個Y * X矩陣               */
/*********************************************************/
int readBMP( char *fileName);        //read file
int saveBMP( char *fileName);        //save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X );        //allocate memory
void Build_mpi_type(BYTE*,BYTE*,BYTE*,MPI_Datatype*);

int main(int argc,char *argv[])
{
/*********************************************************/
/*變數宣告：                                             */
/*  *infileName  ： 讀取檔名                             */
/*  *outfileName ： 寫入檔名                             */
/*  startwtime   ： 記錄開始時間                         */
/*  endwtime     ： 記錄結束時間                         */
/*********************************************************/
        int    myid, numprocs;
        double startwtime = 0.0, endwtime;
        int    namelen;
        char   processor_name[MPI_MAX_PROCESSOR_NAME];

	char *infileName = "input.bmp";
        char *outfileName = "output_broadcast_mpi.bmp";


	MPI_Init(&argc,&argv);
        MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
        MPI_Comm_rank(MPI_COMM_WORLD,&myid);
        
        MPI_Get_processor_name(processor_name,&namelen);
        fprintf(stdout,"Process %d of %d is on %s\n",myid, numprocs, processor_name);
        fflush(stdout);
        MPI_Datatype rgb_type;
        
        
        RGBTRIPLE **send_data=NULL;
        int length_of_each;
        int height_of_each;
        int height;
        int width;
        
	//讀取檔案
        MPI_Type_contiguous(3,MPI_BYTE,&rgb_type);
        MPI_Type_commit(&rgb_type);
        
	//記錄開始時間
        if(myid == 0){
                startwtime = MPI_Wtime(); 
                if ( readBMP( infileName) )
                        cout << "Read file successfully!!" << endl;
                else
                        cout << "Read file fails!!" << endl;
                height = bmpInfo.biHeight;
                width = bmpInfo.biWidth;
        }
        //動態分配記憶體給暫存空間
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&height,1,MPI_INT,0,MPI_COMM_WORLD); 
        MPI_Bcast(&width,1,MPI_INT,0,MPI_COMM_WORLD);
        if(myid != 0){
                BMPSaveData = alloc_memory( height, width);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&BMPSaveData[0][0],height*width,rgb_type,0,MPI_COMM_WORLD);
        
        length_of_each = height*width / numprocs;
        height_of_each = height / numprocs;

        BMPData = alloc_memory( height, width);
        
        
        
        send_data = alloc_memory( height_of_each,width);
        //進行多次的平滑運算
        int vtor[4]={length_of_each,length_of_each,length_of_each,length_of_each};
	int disp[4]={length_of_each*0,length_of_each*1,length_of_each*2,length_of_each*3};
        
        
	for(int count = 0; count < NSmooth ; count ++){
                
		//把像素資料與暫存指標做交換
                
                swap(BMPSaveData,BMPData);
		//進行平滑運算
                //MPI_Scatterv(*BMPData,vtor,disp,rgb_type,*reveive_data,length_of_each,rgb_type,0,MPI_COMM_WORLD);
		for(int i = height_of_each * myid; i<height_of_each * myid+height_of_each ; i++){
			for(int j =0; j<width ; j++){
                                BMPSaveData[i][j].rgbBlue=100;
                                BMPSaveData[i][j].rgbGreen=100;
                                BMPSaveData[i][j].rgbRed=100;
				/*********************************************************/
				/*設定上下左右像素的位置                                 */
				/*********************************************************/
				int Top = i>0 ? i-1 : height-1;
				int Down = i<height-1 ? i+1 : 0;
				int Left = j>0 ? j-1 : width-1;
				int Right = j<width-1 ? j+1 : 0;
				/*********************************************************/
				/*與上下左右像素做平均，並四捨五入                       */
				/*********************************************************/
				send_data[i-height_of_each * myid][j].rgbBlue =  (double) (BMPData[i][j].rgbBlue+BMPData[Top][j].rgbBlue+BMPData[Top][Left].rgbBlue+BMPData[Top][Right].rgbBlue+BMPData[Down][j].rgbBlue+BMPData[Down][Left].rgbBlue+BMPData[Down][Right].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/9+0.5;
				send_data[i-height_of_each * myid][j].rgbGreen =  (double) (BMPData[i][j].rgbGreen+BMPData[Top][j].rgbGreen+BMPData[Top][Left].rgbGreen+BMPData[Top][Right].rgbGreen+BMPData[Down][j].rgbGreen+BMPData[Down][Left].rgbGreen+BMPData[Down][Right].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/9+0.5;
				send_data[i-height_of_each * myid][j].rgbRed =  (double) (BMPData[i][j].rgbRed+BMPData[Top][j].rgbRed+BMPData[Top][Left].rgbRed+BMPData[Top][Right].rgbRed+BMPData[Down][j].rgbRed+BMPData[Down][Left].rgbRed+BMPData[Down][Right].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/9+0.5;
                                
                        }
                        //cout << i << endl;
                }
                //MPI_Barrier(MPI_COMM_WORLD);
                MPI_Barrier(MPI_COMM_WORLD);
                
                //MPI_Gather(&send_data[0][0],length_of_each,rgb_type,&BMPSaveData[0][0],length_of_each,rgb_type,0,MPI_COMM_WORLD);  
                MPI_Gatherv(&send_data[0][0],length_of_each,rgb_type,&BMPSaveData[0][0],vtor,disp,rgb_type,0,MPI_COMM_WORLD);
                //MPI_Barrier(MPI_COMM_WORLD);
                MPI_Bcast(&BMPSaveData[0][0],numprocs*length_of_each,rgb_type,0,MPI_COMM_WORLD); 
	}
        
	//得到結束時間，並印出執行時間
        
 	//寫入檔案
        if(myid == 0){
                endwtime = MPI_Wtime();
    	        cout << "The execution time = "<< endwtime-startwtime <<endl ;
                if ( saveBMP( outfileName ) )
                        cout << "Save file successfully!!" << endl;
                else
                        cout << "Save file fails!!" << endl;
                
        }

        free(send_data);
	free(BMPData[0]);
 	free(BMPSaveData[0]);
 	free(BMPData);
 	free(BMPSaveData);
        MPI_Type_free(&rgb_type);
 	MPI_Finalize();

    return 0;
}

/*********************************************************/
/* 定義mpi變數型態                                        */
/*********************************************************/
void Build_mpi_type(BYTE* r,BYTE* g,BYTE* b,MPI_Datatype* input_mpi)
{

}

/*********************************************************/
/* 讀取圖檔                                              */
/*********************************************************/
int readBMP(char *fileName)
{
	//建立輸入檔案物件
        ifstream bmpFile( fileName, ios::in | ios::binary );

        //檔案無法開啟
        if ( !bmpFile ){
                cout << "It can't open file!!" << endl;
                return 0;
        }

        //讀取BMP圖檔的標頭資料
    	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );

        //判決是否為BMP圖檔
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }

        //讀取BMP的資訊
        bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );

        //判斷位元深度是否為24 bits
        if ( bmpInfo.biBitCount != 24 ){
                cout << "The file is not 24 bits!!" << endl;
                return 0;
        }

        //修正圖片的寬度為4的倍數
        while( bmpInfo.biWidth % 4 != 0 )
        	bmpInfo.biWidth++;

        //動態分配記憶體
        BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);

        //讀取像素資料
    	//for(int i = 0; i < bmpInfo.biHeight; i++)
        //	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	    bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);

        //關閉檔案
        bmpFile.close();

        return 1;

}
/*********************************************************/
/* 儲存圖檔                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
 	//判決是否為BMP圖檔
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }

 	//建立輸出檔案物件
        ofstream newFile( fileName,  ios:: out | ios::binary );

        //檔案無法建立
        if ( !newFile ){
                cout << "The File can't create!!" << endl;
                return 0;
        }

        //寫入BMP圖檔的標頭資料
        newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//寫入BMP的資訊
        newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

        //寫入像素資料
        //for( int i = 0; i < bmpInfo.biHeight; i++ )
        //        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
        newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

        //寫入檔案
        newFile.close();

        return 1;

}


/*********************************************************/
/* 分配記憶體：回傳為Y*X的矩陣                           */
/*********************************************************/
RGBTRIPLE **alloc_memory(int Y, int X )
{
	//建立長度為Y的指標陣列
        RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
	    RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
        memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
        memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//對每個指標陣列裡的指標宣告一個長度為X的陣列
        for( int i = 0; i < Y; i++){
                temp[ i ] = &temp2[i*X];
        }

        return temp;

}
/*********************************************************/
/* 交換二個指標                                          */
/*********************************************************/
void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
	RGBTRIPLE *temp;
	temp = a;
	a = b;
	b = temp;
}
