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
    vector<BlobInfo> result = RegionPartitionTopology(ImgThres);

    Point2f pt=result[0].Center();

    return 0;
}
