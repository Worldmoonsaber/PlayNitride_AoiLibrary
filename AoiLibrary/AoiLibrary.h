#pragma once
#include "OpenCV_Extension_Tool.h"

#pragma region 資料結構

typedef struct
{
	int interval[4];
	int xpitch[3]; //in unit of pixel  xpitch[0]=250pixel
	int ypitch[3];
	int carx;
	int cary;

}SettingP;

typedef struct
{

	double TDwidth; //135
	double TDmaxW;  //1.25
	double TDminW;  //0.9
	double TDheight; //75
	double TDmaxH;   //1.5
	double TDminH;   //0.7

}sizeTD;

typedef struct
{
	int thresmode;
	int bgmax[3];
	int bgmin[3];
	int fgmax[3];
	int fgmin[3];
}thresP;

typedef struct
{
	int PICmode;
	int Outputmode;
	int cols; //1500
	int rows; //1500    
	double correctTheta;
}ImgP;

#pragma endregion


#pragma region Blob 切割

_declspec(dllexport) class BlobInfo;

_declspec(dllexport) void RegionPartition(Mat ImgBinary, vector<BlobInfo>& result);

_declspec(dllexport) BlobInfo* BlobPartition(Mat ImgBinary);

#pragma endregion

#pragma region 影像切割與尋找邊緣

_declspec(dllexport) Point find_piccenter(Mat src);

_declspec(dllexport) Mat CropIMG(Mat img, Rect size);

_declspec(dllexport) int findBoundary(Mat creteriaIMG, Rect inirect, char direction);

_declspec(dllexport) std::tuple<Rect, Point>FindMaxInnerRect(Mat src, Mat colorSRC, sizeTD target, Point TDcenter);


#pragma endregion

#pragma region 影像強化處理

_declspec(dllexport) Mat ContrastEnhancement(Mat srcimg, float alpha, float beta);

_declspec(dllexport) void gammaCorrection(const Mat& src, Mat& dst, const float gamma);

_declspec(dllexport) Mat KmeanOP(int knum, Mat src);

#pragma endregion

_declspec(dllexport) void funcRotatePoint(vector<Point> vPt, vector<Point>& vPtOut, Mat& marksize, float correctTheta, Point IMGoffset);

_declspec(dllexport) Mat RotatecorrectImg(double Rtheta, Mat src);

_declspec(dllexport) void funcThreshold(Mat ImgInput, Mat& ImgThres, thresP thresParm, ImgP imageParm, sizeTD target);

_declspec(dllexport) std::tuple<int, Point_<int>> FindMF_pixel(Mat histImg);

_declspec(dllexport) std::tuple<Mat, Mat, Mat>Histplotting(Mat src, int hist_w, int hist_h, int histsize);
