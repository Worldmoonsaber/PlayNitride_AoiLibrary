// ConsoleCallDllTestApplication.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//
#include <opencv2/core/base.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "AoiLibrary.h"
#include <iostream>
using namespace cv;
using namespace std;
int main()
{
    std:: cout << "Hello World!\n";
    Mat ttt;

    Mat rawimg = imread("C:\\Image\\Pair Chip\\20240830 PN177 chips image\\3_I140.bmp");

    Mat Gimg;
    Mat ImgThres;


    thresP thresParm;
    ImgP imageParm;
    sizeTD target;

    imageParm.cols = 1500; //800 ;900-1600
    imageParm.rows = 1500;
    imageParm.Outputmode = 0; //0:center coord ; 1: multiple mode
    imageParm.PICmode = 0;


    target.TDwidth = 150;
    target.TDmaxW = 1.5;
    target.TDminW = 0.8;

    target.TDheight = 270;
    target.TDmaxH = 1.5;
    target.TDminH = 0.7;

    thresParm = { 3,{208,99999,99999},{99999,99999,99999} ,{9,99999,99999}, {99999,99999,99999} };
    thresParm.thresmode = 5;
    thresParm.fgmax[0] = 1;
    thresParm.bgmax[0] = 101;


    funcThreshold(rawimg,ImgThres,thresParm,imageParm,target);

    BlobInfo* lstPtr;

   // vector<BlobInfo> lst;

    lstPtr=BlobPartition(ImgThres);

    vector<BlobInfo> result = RegionPartitionTopology(ImgThres);

 //   memcpy((void*)&result[0], (void*)&lstPtr, result.size());

    return 0;
}

// 執行程式: Ctrl + F5 或 [偵錯] > [啟動但不偵錯] 功能表
// 偵錯程式: F5 或 [偵錯] > [啟動偵錯] 功能表

// 開始使用的提示: 
//   1. 使用 [方案總管] 視窗，新增/管理檔案
//   2. 使用 [Team Explorer] 視窗，連線到原始檔控制
//   3. 使用 [輸出] 視窗，參閱組建輸出與其他訊息
//   4. 使用 [錯誤清單] 視窗，檢視錯誤
//   5. 前往 [專案] > [新增項目]，建立新的程式碼檔案，或是前往 [專案] > [新增現有項目]，將現有程式碼檔案新增至專案
//   6. 之後要再次開啟此專案時，請前往 [檔案] > [開啟] > [專案]，然後選取 .sln 檔案
