#include "AOILib_MTUkey_V1.h"


std::tuple<int, Mat, Point, Mat> FindKey_Square(int notFoundReason, Mat src, Mat grayimg, sizeTD target, thresP thresParm, SettingP chipsetting)
{

    /*Output parm */
    Point crossCenter;

    /*Parm 1. image pre-processing*/
    Mat EnhanceIMG;
    float alpha = 2; //2
    float beta = -50;//-50

    /*Parm 2. auto-thres filter */
    int adaptWsize = 235; //235
    int adaptKsize = 4;  //8
    adaptWsize = thresParm.bgmax[0];
    adaptKsize = thresParm.fgmax[0];


    int elementSize = thresParm.bgmax[1];

    if (elementSize < 1 || elementSize >= 9999)
        elementSize = 20; // 20 as Default

    Mat thresImg;
    Mat element = getStructuringElement(MORPH_RECT, Size(elementSize, elementSize));
    vector <vector<Point>>contours;
    int thresfilter = THRESH_BINARY_INV; //THRESH_BINARY_INV

    if (thresParm.thresmode == 4)
    {
        thresfilter = THRESH_BINARY;
    }
    else if (thresParm.thresmode == 3)
    {
        thresfilter = THRESH_BINARY_INV;
    }


    Mat resizesrc = Mat::zeros(Size(1773, 1533), CV_8UC3);
    resize(src, resizesrc, Size(1773, 1533), 0, 0, INTER_LINEAR); //INTER_CUBIC
    Mat resizethres = Mat::zeros(Size(1773, 1533), CV_8UC1);


    /*thresParm input*/
    Mat c1 = Mat::zeros(src.size(), CV_8UC1);
    Mat c2 = Mat::zeros(src.size(), CV_8UC1);
    Mat Anspic = Mat::zeros(src.size(), CV_8UC1);
    Mat thresRot;
    Mat marksize = Mat::ones(src.rows, src.cols, CV_8UC3);
    src.copyTo(marksize);

    vector <vector<Point>> contRot, contours2, testcontours;

    Point Offsetpt = Point(0, 0);

    //vector<RotatedRect> testRotCnt;
    vector<double> testRotArea;
    vector <vector<Point>> potentialcnt;
    vector<Point> approx;
    cv::RotatedRect RottargetRect;
    Rect bdRotRect;

    Mat hullpattern = Mat::zeros(src.size(), CV_8UC1);

    ////Step 1. image pre-processing::

    EnhanceIMG = ContrastEnhancement(grayimg, alpha, beta);

    if (thresParm.bgmax[0] & 1)
    {
        adaptWsize = thresParm.bgmax[0];
        adaptKsize = thresParm.fgmax[0];
    }
    else
    {
        adaptWsize = thresParm.bgmax[0] + 1;
        adaptKsize = thresParm.fgmax[0];
    }


    ////Step 2. auto-thres filter::
    cv::adaptiveThreshold(EnhanceIMG, thresImg, 255, ADAPTIVE_THRESH_MEAN_C, thresfilter, adaptWsize, adaptKsize);

    cv::medianBlur(thresImg, thresImg, 9);

#pragma region Canny 邊緣影像 只針對Threshold影像的邊緣進行強化處理 (在背景與Key顏色相近時 特別有用 其他時間作用不大)

    Mat edges;
    cv::Canny(thresImg, edges, adaptWsize, 200);//觸發條件其實很嚴苛,大部分時候 都是全黑影像
    cv::morphologyEx(edges, edges, MORPH_CLOSE, element, Point(-1, -1));
    cv::morphologyEx(edges, edges, MORPH_OPEN, element, Point(-1, -1));
    cv::morphologyEx(edges, edges, MORPH_ERODE, element, Point(-1, -1));

#pragma endregion



    cv::morphologyEx(thresImg, thresImg, MORPH_CLOSE, element, Point(-1, -1));
    cv::rectangle(thresImg, Rect(0, 0, src.cols, src.rows), Scalar(0, 0, 0), 2);
    thresImg = thresImg - edges;//刪除絕對不可能是Key的區域

    if (!edges.empty())
        edges.release();

    float ref_Ratio = 1;

    if (target.TDwidth < target.TDheight)
        ref_Ratio = target.TDheight / target.TDwidth;
    else
        ref_Ratio = target.TDwidth / target.TDheight;


    ////Step 3. dimension filter::
    //// 20240921 進行重構
    vector<BlobInfo> vRegions = RegionPartitionTopology(thresImg);

    if (vRegions.size() == 0)
    {
        notFoundReason = 1;
        throw "something wrong::threshold value issue";
    }



    vector<BlobInfo> vRegionsPossibleKey;

    //-----小角度條件搜尋
    for (int i = 0; i < vRegions.size(); i++)
    {
        if (vRegions[i].Area() < 1)
            continue;

        if (vRegions[i].AspectRatio() > 1.05 * ref_Ratio)
            continue;

        if (vRegions[i].AspectRatio() < 0.95 * ref_Ratio)
            continue;

        if (vRegions[i].Width() > target.TDminW * target.TDwidth && vRegions[i].Height() > target.TDminH * target.TDheight &&
            vRegions[i].Width() <= target.TDmaxW * target.TDwidth && vRegions[i].Height() <= target.TDmaxH * target.TDheight)
        {
            vRegionsPossibleKey.push_back(vRegions[i]);
        }
    }

    //-----未來如果轉台可以大角度旋轉 (>10度)再加入大角度搜尋


    ////Step 3. dimension filter::
    //cout << "check cnt size:: " << contours.size() << endl;
    try
    {
        if (vRegionsPossibleKey.size() == 0)
        {
            notFoundReason = 2;
            throw "something wrong::demension value issue or FOV does not contain key";
        }
        else
        {

            ////Step 4. remove the inner replicated key::
            int finemode = 0; //finemode=0:Rectangle / finemode=1:Circle

            if (vRegionsPossibleKey.size() == 0)
            {
                notFoundReason = 2;
                throw "something wrong::demension value issue or FOV does not contain key";
            }
            else if (vRegionsPossibleKey.size() == 1)
            {
                RottargetRect = minAreaRect(vRegionsPossibleKey[0].contourMain());
                bdRotRect = boundingRect(vRegionsPossibleKey[0].contourMain());
            }
            else
            {
                //---判斷所有中心位置最遠距離
                float centerDist = -1;

                for (int i = 0; i < vRegionsPossibleKey.size(); i++)
                    for (int j = i + 1; j < vRegionsPossibleKey.size(); j++)
                    {
                        RotatedRect minrect1 = minAreaRect(vRegionsPossibleKey[i].contourMain());
                        RotatedRect minrect2 = minAreaRect(vRegionsPossibleKey[j].contourMain());

                        float dist_Tmp = norm(minrect1.center - minrect2.center);

                        if (dist_Tmp > centerDist)
                            centerDist = dist_Tmp;
                    }



                //------判斷是否同時多個位置
                float ref_Length;

                if (target.TDwidth < target.TDheight)
                    ref_Length = target.TDwidth;
                else
                    ref_Length = target.TDheight;


                if (0.3 * ref_Length < centerDist)
                {
                    finemode = -1;
                }
                else
                {
                    //---判斷何者與目標舉行最相似
                    std::sort(vRegionsPossibleKey.begin(), vRegionsPossibleKey.end(), [&, target](BlobInfo& a, BlobInfo& b)
                        {

                            float refArea = target.TDheight * target.TDwidth;

                            float rectA_Similar;
                            float rectB_Similar;

                            if (a.minRectHeight() * a.minRectWidth() > refArea)
                                rectA_Similar = a.minRectHeight() * a.minRectWidth() / refArea;
                            else
                                rectA_Similar = refArea / a.minRectHeight() * a.minRectWidth();

                            if (b.minRectHeight() * b.minRectWidth() > refArea)
                                rectB_Similar = b.minRectHeight() * b.minRectWidth() / refArea;
                            else
                                rectB_Similar = refArea / b.minRectHeight() * b.minRectWidth();


                            return rectA_Similar < rectB_Similar;
                        });

                    RottargetRect = minAreaRect(vRegionsPossibleKey[0].contourMain());
                    bdRotRect = boundingRect(vRegionsPossibleKey[0].contourMain());
                }
            }

            /*else
            {
                notFoundReason = 6;
                throw "something wrong66::more than one key in FOV or defect key";
            }*/



            cv::circle(src, RottargetRect.center, 2, Scalar(255, 40, 10), 5);

            std::cout << "check finemode is : " << finemode << endl;
            ////Step 5. define center on the basis of key pattern(circle or rectangle)::
            if (finemode == 1) //circle
            {

                crossCenter = RottargetRect.center + Point2f(chipsetting.carx, chipsetting.cary);

                cv::line(marksize, Point(0, crossCenter.y), Point(marksize.cols, crossCenter.y), Scalar(0, 255, 255), 2);
                cv::line(marksize, Point(crossCenter.x, 0), Point(crossCenter.x, marksize.rows), Scalar(0, 255, 255), 2);

                cv::circle(marksize,
                    (crossCenter), //coordinate
                    2, //radius
                    Scalar(255, 255, 255),  //color
                    -1);
                cv::circle(src, crossCenter, 3, Scalar(30, 88, 160), -1);



            }

            else if (finemode == 0) //rectangle
            {

                Point offsetPt = Point(bdRotRect.x, bdRotRect.y);

                crossCenter = Point(RottargetRect.center);// +Point(chipsetting.carx, chipsetting.cary);

                Point2f vertices[4];
                RottargetRect.points(vertices);

                line(marksize, vertices[0] + Point2f(chipsetting.carx, chipsetting.cary), vertices[1] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(0, 255, 255), 3);
                line(marksize, vertices[1] + Point2f(chipsetting.carx, chipsetting.cary), vertices[2] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(0, 255, 255), 3);
                line(marksize, vertices[2] + Point2f(chipsetting.carx, chipsetting.cary), vertices[3] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(0, 255, 255), 3);
                line(marksize, vertices[3] + Point2f(chipsetting.carx, chipsetting.cary), vertices[0] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(0, 255, 255), 3);

                crossCenter = Point(RottargetRect.center) + Point(chipsetting.carx, chipsetting.cary);


                for (int i = 0; i < 4; i++)
                    line(marksize, vertices[i] + Point2f(chipsetting.carx, chipsetting.cary), crossCenter, Scalar(0, 255, 255), 3);


                element.release();
                Anspic.release();
                c1.release();
                c2.release();
                EnhanceIMG.release();
                grayimg.release();

            }
            else if (finemode == -1)
            {
                for (int i = 0; i < vRegionsPossibleKey.size(); i++)
                {
                    RottargetRect = minAreaRect(vRegionsPossibleKey[i].contourMain());

                    Point2f vertices[4];
                    RottargetRect.points(vertices);

                    line(marksize, vertices[0], vertices[1], Scalar(0, 255, 255), 3);
                    line(marksize, vertices[1], vertices[2], Scalar(0, 255, 255), 3);
                    line(marksize, vertices[2], vertices[3], Scalar(0, 255, 255), 3);
                    line(marksize, vertices[3], vertices[0], Scalar(0, 255, 255), 3);



                    for (int i = 0; i < 4; i++)
                        line(marksize, vertices[i], RottargetRect.center, Scalar(0, 255, 255), 3);

                }

                crossCenter = Point(0, 0);// +Point(chipsetting.carx, chipsetting.cary);

                notFoundReason = 6;
                //throw "something wrong66::more than one key in FOV or defect key";
            }

            if (crossCenter.x > chipsetting.xpitch[0] && src.cols - chipsetting.xpitch[0] > crossCenter.x
                && crossCenter.y > chipsetting.ypitch[0] && src.rows - chipsetting.ypitch[0] > crossCenter.y)
            {
                notFoundReason = 9;
            }
            else
            {
                notFoundReason = 4;
            }


        }
    }

    catch (const char* message)
    {
        std::cout << message << std::endl;

    }


    cv::resize(marksize, resizesrc, Size(1773, 1533), 0, 0, INTER_LINEAR);  /*(5320/3,4600/3)*/
    cv::resize(thresImg, resizethres, Size(1773, 1533), 0, 0, INTER_LINEAR);  /*(5320/3,4600/3)*/



    std::cout << "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*" << endl;
    std::cout << "check key crossCenternew is: [ " << crossCenter << " ]" << endl;
    std::cout << "check key state  is: [ " << notFoundReason << " ]" << endl;
    std::cout << "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*" << endl;

    hullpattern.release();
    marksize.release();
    src.release();
    thresImg.release();

    return { notFoundReason, resizethres, crossCenter, resizesrc };
}

std::tuple<int, Mat, Point, Mat> FindKey_Cross(int notFoundReason, Mat src, Mat grayimg, sizeTD target, thresP thresParm, SettingP chipsetting)
{


    vector<Scalar> markcolorn = { Scalar(255, 255, 0),Scalar(60, 60, 60),Scalar(0, 90, 255) };


    /*Output parm */
    Point crossCenter;


    /*Parm 1. image pre-processing*/
    Mat EnhanceIMG;
    float alpha = 2; //1.5
    float beta = -50;

    /*Parm 2. auto-thres filter */
    int adaptWsize = 100; //235
    int adaptKsize = 4;  //8
    adaptWsize = thresParm.bgmax[0];
    adaptKsize = thresParm.fgmax[0];
    Mat thresImg;
    Mat element = getStructuringElement(MORPH_RECT, Size(20, 20));
    vector <vector<Point>>contours;

    Mat resizesrc = Mat::zeros(Size(1773, 1533), CV_8UC3);;
    cv::resize(src, resizesrc, Size(1773, 1533), 0, 0, INTER_LINEAR); //INTER_CUBIC
    Mat resizethres = Mat::zeros(Size(1773, 1533), CV_8UC1);


    /*thresParm input*/
    int thresfiltermode = THRESH_BINARY;
    Mat c1 = Mat::zeros(src.size(), CV_8UC1);
    Mat c2 = Mat::zeros(src.size(), CV_8UC1);
    Mat Anspic = Mat::zeros(src.size(), CV_8UC1);
    Mat thresRot;
    Mat marksize = Mat::ones(src.rows, src.cols, CV_8UC3);
    src.copyTo(marksize);

    vector <vector<Point>> contRot, contours2, testcontours;

    Point Offsetpt = Point(0, 0);

    vector<RotatedRect> testRotCnt;
    vector <vector<Point>> potentialcnt;
    vector<Point> approx;
    cv::RotatedRect RottargetRect;
    Rect bdRotRect;


    ////Step 1. image pre-processing::

    EnhanceIMG = ContrastEnhancement(grayimg, alpha, beta);


    ////Step 2. auto-thres filter::
    //adaptiveThreshold(EnhanceIMG, thresImg, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, adaptWsize, adaptKsize);
    if (thresParm.thresmode == 4)
    {
        thresfiltermode = THRESH_BINARY;
    }
    else if (thresParm.thresmode == 3)
    {
        thresfiltermode = THRESH_BINARY_INV;
    }

    if (thresParm.bgmax[0] & 1)
    {
        adaptWsize = thresParm.bgmax[0];
        adaptKsize = thresParm.fgmax[0];
    }
    else
    {
        adaptWsize = thresParm.bgmax[0] + 1;
        adaptKsize = thresParm.fgmax[0];
    }
    adaptiveThreshold(EnhanceIMG, thresImg, 255, ADAPTIVE_THRESH_MEAN_C, thresfiltermode, adaptWsize, adaptKsize);
    cv::medianBlur(thresImg, thresImg, 9);
    morphologyEx(thresImg, thresImg, MORPH_CLOSE, element, Point(-1, -1));
    cv::rectangle(thresImg, Rect(0, 0, src.cols, src.rows), Scalar(0, 0, 0), 2);
    findContours(thresImg, contours, RETR_CCOMP, CHAIN_APPROX_SIMPLE, Point());//RETR_TREE  /RETR_CCOMP  //RETR_EXTERNAL


    ////Step 3. dimension filter::
    //cout << "check cnt size:: " << contours.size() << endl;
    try
    {
        if (contours.size() == 0)
        {
            notFoundReason = 1;
            throw "something wrong::threshold value issue";
        }
        else
        {
            for (int i = 0; i < contours.size(); i++) {

                Mat boxPointsF2, boxPointsInt2;
                //vector<Point> approx;
                cv::RotatedRect min_rect = minAreaRect(contours[i]);
                float width = min_rect.size.width;
                float height = min_rect.size.height;

                cv::approxPolyDP(contours[i], approx, 15, true);


                if (width > target.TDminW * target.TDwidth && height > target.TDminH * target.TDheight &&
                    width <= target.TDmaxW * target.TDwidth && height <= target.TDmaxH * target.TDheight
                    && approx.size() < 25 //avoid inner duplicate  definition > 17
                    )
                {
                    //torrence
                    testRotCnt.push_back(min_rect);
                    testcontours.push_back(contours[i]);
                    cout << "check approx size:: " << approx.size() << "/*/*/ " << width << " */*/*/" << height << endl;

                    //cv::boxPoints(min_rect, boxPointsF2);  //rotated rectangle converts to four points
                    //boxPointsF2.assignTo(boxPointsInt2, CV_32S);//point2f to 32int : 643.55444(boxPoints2f)-->644
                    ////cv::polylines(src, boxPointsInt2, true, Scalar(255, 255, 255), -1); //depict frame



                    Point2f vertices[4];
                    min_rect.points(vertices);

                    line(marksize, vertices[0] + Point2f(chipsetting.carx, chipsetting.cary), vertices[1] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(20, 20, 20), 2);
                    line(marksize, vertices[1] + Point2f(chipsetting.carx, chipsetting.cary), vertices[2] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(20, 20, 20), 2);
                    line(marksize, vertices[2] + Point2f(chipsetting.carx, chipsetting.cary), vertices[3] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(20, 20, 20), 2);
                    line(marksize, vertices[3] + Point2f(chipsetting.carx, chipsetting.cary), vertices[0] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(20, 20, 20), 2);


                    line(src, vertices[0] + Point2f(chipsetting.carx, chipsetting.cary), vertices[1] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(20, 20, 20), 2);
                    line(src, vertices[1] + Point2f(chipsetting.carx, chipsetting.cary), vertices[2] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(20, 20, 20), 2);
                    line(src, vertices[2] + Point2f(chipsetting.carx, chipsetting.cary), vertices[3] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(20, 20, 20), 2);
                    line(src, vertices[3] + Point2f(chipsetting.carx, chipsetting.cary), vertices[0] + Point2f(chipsetting.carx, chipsetting.cary), Scalar(20, 20, 20), 2);




                    //cv::polylines(src, boxPointsInt2 , true, Scalar(20, 20, 20), 2);
                    //cv::polylines(marksize, boxPointsInt2 , true, Scalar(20, 20, 20), 2);
                    ////cv::fillPoly(src, boxPointsInt2, Scalar(255, 255, 255));  //fill area
                    //cout << "check approx " << approx.size() << endl;
                    drawContours(c1, contours, i, Scalar(255, 255, 255), -1);

                }
            }

            ////Step 4. remove the inner replicated key::
            int finemode = 0; //finemode=0:Rectangle / finemode=1:Circle

            if (testcontours.size() == 0)
            {
                notFoundReason = 2;
                throw "something wrong::demension value issue or FOV does not contain key";
            }
            else //if (testcontours.size() == 1)
            {
                Mat boxPointsF3, boxPointsInt3;

                cout << "start to fine positioning....." << endl;



                RottargetRect = minAreaRect(testcontours[0]);
                Moments M = (moments(testcontours[0], false));
                crossCenter = ((Point((M.m10 / M.m00), (M.m01 / M.m00)))) + Point(chipsetting.carx, chipsetting.cary);;
                Point2f vertices[4];
                RottargetRect.points(vertices);


                for (int i = 0; i < 4; i++)
                {

                    line(marksize, vertices[i] + Point2f(chipsetting.carx, chipsetting.cary), crossCenter, Scalar(20, 20, 20), 2);
                }

                cv::circle(marksize, crossCenter, 5, Scalar(160, 80, 200), 10);



            }


            cv::circle(src, RottargetRect.center, 3, Scalar(255, 40, 10), -1);


            if (crossCenter.x > chipsetting.xpitch[0] && src.cols - chipsetting.xpitch[0] > crossCenter.x
                && crossCenter.y > chipsetting.ypitch[0] && src.rows - chipsetting.ypitch[0] > crossCenter.y)
            {
                notFoundReason = 9;
            }
            else
            {
                notFoundReason = 4;
            }


            cv::circle(src, crossCenter, 3, Scalar(30, 88, 160), -1);








            std::cout << "check key crossCenternew is: [ " << crossCenter << " ]" << endl;
            std::cout << "check key state  is: [ " << notFoundReason << " ]" << endl;
            std::cout << "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*" << endl;



        }




    }

    catch (const char* message)
    {
        std::cout << message << std::endl;

    }

    cv::resize(marksize, resizesrc, Size(1773, 1533), 0, 0, INTER_LINEAR); //INTER_CUBIC  /*(5320/3,4600/3)*/
    cv::resize(thresImg, resizethres, Size(1773, 1533), 0, 0, INTER_LINEAR); //INTER_CUBIC  /*(5320/3,4600/3)*/



    std::cout << "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*" << endl;
    std::cout << "check key crossCenternew is: [ " << crossCenter << " ]" << endl;
    std::cout << "check key state  is: [ " << notFoundReason << " ]" << endl;
    std::cout << "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*" << endl;

    return { notFoundReason, resizethres, crossCenter, resizesrc };
}

void MTUkey_calcenter(thresP thresParm, ImgP imageParm, SettingP chipsetting, sizeTD target, unsigned int* imageIN,
					unsigned int* imageOUT, unsigned char* imageGray, float boolResult[], float outputLEDX[], float outputLEDY[])
{
	Mat rawimg, cropedRImg, gauBGR;
	Mat Gimg, drawF2;
	
	Point IMGoffset=Point(0,0);

	//output parameters::
	Point2f crossCenter;
	int boolflag = 0;

	Mat image_input(imageParm.rows, imageParm.cols, CV_8UC4, &imageIN[0]); // THIS IS THE INPUT IMAGE, POINTER TO DATA			
	image_input.copyTo(rawimg);

	Mat image_output(1533, 1773, CV_8UC4, &imageOUT[0]);
	Mat thres_output(1533, 1773, CV_8UC1, &imageGray[0]);

	try
	{
		if (rawimg.empty())
		{
			boolflag = 8;
			throw "something wrong::input image failure";
		} //check if image is empty

	} //try loop
	catch (const char* message)
	{

		std::cout << "check catch state:: " << boolflag << endl;


	}//catch loop

	

	if (boolflag == 0) //&& imageParm.Outputmode == 0
	{

		/*image with CROP  process :::*/
		//Point piccenter;
		//piccenter = find_piccenter(rawimg);
		////std::cout << "pic center is ::" << piccenter.x << " , " << piccenter.y << endl;	
		//IMGoffset.x = piccenter.x - int(imageParm.cols * 0.5);  //2736-600*0.5=2476
		//IMGoffset.y = piccenter.y - int(imageParm.rows * 0.5);  //1824-600*0.5=1564
		//Rect Cregion(IMGoffset.x, IMGoffset.y, imageParm.cols, imageParm.rows);
		//cropedRImg = CropIMG(rawimg, Cregion);

		///*///*image without CROP  process :::*/
		//sizeParm.CsizeW = rawimg.size[0];
		//sizeParm.CsizeH = sizeParm.CsizeW;
		rawimg.copyTo(cropedRImg);

        cvtColor(cropedRImg, cropedRImg, COLOR_RGB2GRAY);


		//start to ISP//////////////////////////

        //std::tie(boolflag, Gimg, crossCenter, drawF2) = ThresFiltering_1211(boolflag, rawimg, cropedRImg, target, thresParm, chipsetting);
        //std::tie(boolflag, Gimg, crossCenter, drawF2) = ThresFiltering_1219(boolflag, rawimg, cropedRImg, target, thresParm, chipsetting);
        if (imageParm.PICmode == 0)
        {
            std::tie(boolflag, Gimg, crossCenter, drawF2) = FindKey_Square(boolflag, rawimg, cropedRImg, target, thresParm, chipsetting);
        }
        else if (imageParm.PICmode == 1)
        {
            std::tie(boolflag, Gimg, crossCenter, drawF2) = FindKey_Cross(boolflag, rawimg, cropedRImg, target, thresParm, chipsetting);
        }
        else
        {
            std::tie(boolflag, Gimg, crossCenter, drawF2) = FindKey_Square(boolflag, rawimg, cropedRImg, target, thresParm, chipsetting);
        }
		
		
	}

	std::cout << "check img state:: " << boolflag << endl;
	std::cout << "check center is ::" << crossCenter << endl;

	

	/*  :::::::OUTPUT area:::::::  */
	outputLEDX[0] = crossCenter.x ;
	outputLEDY[0] = crossCenter.y ;
	Gimg.copyTo(thres_output);
	drawF2.copyTo(image_output);
	boolResult[0] = boolflag;
}
