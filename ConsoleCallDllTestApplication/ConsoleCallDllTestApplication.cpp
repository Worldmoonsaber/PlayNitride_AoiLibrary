// ConsoleCallDllTestApplication.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//
#include <opencv2/core/base.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include "AoiLibrary.h"

using namespace cv;
using namespace std;


vector<BlobInfo> FindSpecificRegionsBySizeTD(Mat ImgBinary, sizeTD szTD)
{
    vector<BlobInfo> vRegion;

    vRegion = RegionPartitionTopology(ImgBinary);

    vector<BlobInfo> results;

    for (int i = 0; i < vRegion.size(); i++)
    {
        if (vRegion[i].Width() > szTD.TDwidth * szTD.TDmaxW || vRegion[i].Width() < szTD.TDwidth * szTD.TDminW)
            continue;

        if (vRegion[i].Height() > szTD.TDheight * szTD.TDmaxH || vRegion[i].Height() < szTD.TDheight * szTD.TDminH)
            continue;

        results.push_back(vRegion[i]);
    }

    return results;
}




int main()
{

    std::cout << "LibVersion="<< LibVersion() << endl;


    Mat imgKey = imread("C:\\Users\\Playuser\\Downloads\\20240930_154751.bmp");

    cvtColor(imgKey, imgKey, COLOR_RGB2GRAY);
    Mat thres;
    threshold(imgKey, thres, 200, 255, THRESH_BINARY_INV);

    sizeTD szTD;

    szTD.TDheight = 250;
    szTD.TDwidth = 270;
    szTD.TDmaxH = 1.5;
    szTD.TDminH = 0.7;
    szTD.TDmaxW = 1.5;
    szTD.TDminW = 0.7;

    

    vector<BlobInfo> results = FindSpecificRegionsBySizeTD(thres, szTD);
        

    std::cout << "results.cOUNT :"<< results.size() << endl;

    Mat imgEmpty = Mat(thres.size(), CV_8UC1);


    system("PAUSE");
    return 0;
}
