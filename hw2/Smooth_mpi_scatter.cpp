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

/*********************************************************/
BMPHEADER bmpHeader;
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;


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
int toppartner(int,int);        //get top partner
int downpartner(int,int);       //get down partner
RGBTRIPLE **alloc_memory( int Y, int X );        //allocate memory
void build_broaden_data(RGBTRIPLE**,RGBTRIPLE**,RGBTRIPLE**,RGBTRIPLE**,int,int);       //build broaden data
int main(int argc,char *argv[])
{
/*********************************************************/
/*變數宣告：                                             */
/*  *infileName  ： 讀取檔名                             */
/*  *outfileName ： 寫入檔名                             */
/*  startwtime   ： 記錄開始時間                         */
/*  endwtime     ： 記錄結束時間                         */
/*********************************************************/
        int    myid, numprocs;          //own id,total process count
        double startwtime = 0.0, endwtime;
        int    namelen;
        char   processor_name[MPI_MAX_PROCESSOR_NAME];

	char *infileName = "input.bmp";
        char *outfileName = "output_scatter_mpi.bmp";

        // MPI init setting
	MPI_Init(&argc,&argv);
        MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
        MPI_Comm_rank(MPI_COMM_WORLD,&myid);
        MPI_Get_processor_name(processor_name,&namelen);
        fprintf(stdout,"Process %d of %d is on %s\n",myid, numprocs, processor_name);
        fflush(stdout);
        
        int length_of_each;     //each node data length
        int height_of_each;     //each node data length
        int height;             //bmp total height
        int width;              //bmp total width
        
	// build MPI type
        MPI_Datatype rgb_type;
        MPI_Type_contiguous(3,MPI_BYTE,&rgb_type);
        MPI_Type_commit(&rgb_type);
        
	//記錄開始時間
        if(myid == 0){
                startwtime = MPI_Wtime(); 
                //讀取檔案
                if ( readBMP( infileName) )
                        cout << "Read file successfully!!" << endl;
                else
                        cout << "Read file fails!!" << endl;
                height = bmpInfo.biHeight;
                width = bmpInfo.biWidth;
        }
        //broadcast height, width, data to other nodes
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

        
        //computing data declare
        RGBTRIPLE **local_data=NULL;            //local data in each node
        RGBTRIPLE **send_data_top=NULL;         //data send to top partner
        RGBTRIPLE **send_data_down=NULL;        //data send to bottom partner
        RGBTRIPLE **recv_data_top=NULL;         //data receive from top partner
        RGBTRIPLE **recv_data_down=NULL;        //data receive from bottom partner
        RGBTRIPLE **broaden_data=NULL;          //data broaden use to compute
        MPI_Request request[4];
        MPI_Status status;
        int top,down;                           //top and bottom partner id 
        // 記憶體分配
        local_data = alloc_memory( height_of_each,width);
        send_data_top = alloc_memory( 1,width);
        send_data_down = alloc_memory( 1,width);
        recv_data_top = alloc_memory( 1,width);
        recv_data_down = alloc_memory( 1,width);
        broaden_data = alloc_memory( height_of_each+2,width);
        
        //Scatterv and Gatherv setting
        int vtor[4]={length_of_each,length_of_each,length_of_each,length_of_each};
	int disp[4]={length_of_each*0,length_of_each*1,length_of_each*2,length_of_each*3};

        //Scatter data to each node
        MPI_Scatterv(&BMPSaveData[0][0],vtor,disp,rgb_type,&local_data[0][0],length_of_each,rgb_type,0,MPI_COMM_WORLD);
        
        //get partner id
        top = toppartner(myid,numprocs);                
        down = downpartner(myid,numprocs);              

        //Smooth computing
	for(int count = 0; count < NSmooth ; count ++){
                //set sending data
                for(int i=0;i<width;i++){
                        send_data_top[0][i] = local_data[0][i];
                        send_data_down[0][i] = local_data[height_of_each-1][i];
                }
                
                //send/receive data to/from partner
                MPI_Isend(&send_data_top[0][0],width,rgb_type,top,0,MPI_COMM_WORLD,&request[0]);
                MPI_Isend(&send_data_down[0][0],width,rgb_type,down,0,MPI_COMM_WORLD,&request[1]);
                MPI_Irecv(&recv_data_down[0][0],width,rgb_type,down,0,MPI_COMM_WORLD,&request[2]);
                MPI_Irecv(&recv_data_top[0][0],width,rgb_type,top,0,MPI_COMM_WORLD,&request[3]);
                MPI_Wait(&request[0],&status);
                MPI_Wait(&request[1],&status);
                MPI_Wait(&request[2],&status);
                MPI_Wait(&request[3],&status);
                
                //build broad data to computef
                build_broaden_data(broaden_data,recv_data_top,local_data,recv_data_down,height_of_each,width);
		//進行平滑運算
		for(int i = 0; i<height_of_each ; i++){
			for(int j =0; j<width ; j++){
				/*********************************************************/
				/*設定上下左右像素的位置                                 */
				/*********************************************************/
				int self = i+1;         
                                int Top = i;
				int Down = i+2;
				int Left = j>0 ? j-1 : width-1;
				int Right = j<width-1 ? j+1 : 0;
				/*********************************************************/
				/*與上下左右像素做平均，並四捨五入                       */
				/*********************************************************/
				local_data[i][j].rgbBlue =  (double) (broaden_data[self][j].rgbBlue+broaden_data[Top][j].rgbBlue+broaden_data[Top][Left].rgbBlue+broaden_data[Top][Right].rgbBlue+broaden_data[Down][j].rgbBlue+broaden_data[Down][Left].rgbBlue+broaden_data[Down][Right].rgbBlue+broaden_data[self][Left].rgbBlue+broaden_data[self][Right].rgbBlue)/9+0.5;
				local_data[i][j].rgbGreen =  (double) (broaden_data[self][j].rgbGreen+broaden_data[Top][j].rgbGreen+broaden_data[Top][Left].rgbGreen+broaden_data[Top][Right].rgbGreen+broaden_data[Down][j].rgbGreen+broaden_data[Down][Left].rgbGreen+broaden_data[Down][Right].rgbGreen+broaden_data[self][Left].rgbGreen+broaden_data[self][Right].rgbGreen)/9+0.5;
				local_data[i][j].rgbRed =  (double) (broaden_data[self][j].rgbRed+broaden_data[Top][j].rgbRed+broaden_data[Top][Left].rgbRed+broaden_data[Top][Right].rgbRed+broaden_data[Down][j].rgbRed+broaden_data[Down][Left].rgbRed+broaden_data[Down][Right].rgbRed+broaden_data[self][Left].rgbRed+broaden_data[self][Right].rgbRed)/9+0.5;
                                
                        }
                }    
	}
        //gather each node compute result to node 0
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Gatherv(&local_data[0][0],length_of_each,rgb_type,&BMPSaveData[0][0],vtor,disp,rgb_type,0,MPI_COMM_WORLD);
        
	
        
 	//寫入檔案
        if(myid == 0){
                //得到結束時間，並印出執行時間
                endwtime = MPI_Wtime();
    	        cout << "The execution time = "<< endwtime-startwtime <<endl ;
                if ( saveBMP( outfileName ) )
                        cout << "Save file successfully!!" << endl;
                else
                        cout << "Save file fails!!" << endl;
                
        }
        free(local_data[0]);
        free(send_data_top[0]);
        free(send_data_down[0]);
        free(recv_data_top[0]);
        free(recv_data_down[0]);
        free(broaden_data[0]);
        free(local_data);
        free(send_data_top);
        free(send_data_down);
        free(recv_data_top);
        free(recv_data_down);
        free(broaden_data);
 	free(BMPSaveData);
 	free(BMPSaveData);
 	MPI_Finalize();

    return 0;
}

/*********************************************************/
/* 建立Broaden array才能計算                              */
/*********************************************************/
void build_broaden_data(RGBTRIPLE** broaden_data,RGBTRIPLE** recv_data_top,RGBTRIPLE** local_data,RGBTRIPLE** recv_data_down,int height_of_each,int width){
        for(int i=0;i<height_of_each+2;i++){
                for(int j=0;j<width;j++){
                        if(i==0){
                                broaden_data[i][j]=recv_data_top[0][j];
                        }else if(i==height_of_each+1){
                                broaden_data[i][j]=recv_data_down[0][j];
                        }else{
                                broaden_data[i][j]=local_data[i-1][j];
                        }
                }
        }
}
/*********************************************************/
/* 取得partner的id                                        */
/*********************************************************/
int toppartner(int myid,int numprocs)
{
        int partner_id;
        if(myid == 0){
                partner_id = numprocs -1;
        }else{
                partner_id = myid -1;
        }
        return partner_id;
}
int downpartner(int myid,int numprocs)
{
        int partner_id;
        if(myid == numprocs-1){
                partner_id = 0;
        }else{
                partner_id = myid +1;
        }
        return partner_id;
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
