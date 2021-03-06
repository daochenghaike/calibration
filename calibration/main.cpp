#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// OpenCV
#include <cxcore.h>
#include <cv.h>
#include <highgui.h>
#include <cvaux.h>

void InitCorners3D(CvMat *Corners3D, CvSize ChessBoardSize, int Nimages, float SquareSize);
void makeChessBoard();
int myFindChessboardCorners( const void* image, CvSize pattern_size,
                             CvPoint2D32f* corners, int* corner_count=NULL,
                             int flags=CV_CALIB_CB_ADAPTIVE_THRESH );


inline int drawCorssMark(IplImage *dst,CvPoint pt)
/*************************************************
  Function:        main_loop
  Description:     绘制一个十字标记
  Calls:
  Called By:
  Input:           RGB image,  pt
  Output:
  Return:
  Others:          需要检查坐标是否越界 to do list
*************************************************/
{

	const int cross_len = 4;
	CvPoint pt1,pt2,pt3,pt4;
	pt1.x = pt.x;
	pt1.y = pt.y - cross_len;
	pt2.x = pt.x;
	pt2.y = pt.y + cross_len;
	pt3.x = pt.x - cross_len;
	pt3.y = pt.y;
	pt4.x = pt.x + cross_len;
	pt4.y = pt.y;

	cvLine(dst,pt1,pt2,CV_RGB(0,255,0),2,CV_AA, 0 );
	cvLine(dst,pt3,pt4,CV_RGB(0,255,0),2,CV_AA, 0 );

	return 0;
}

/* declarations for OpenCV */
IplImage                 *current_frame_rgb,grid;
IplImage                 *current_frame_gray;
IplImage                 *chessBoard_Img;

int                       Thresholdness = 120;

int image_width = 320;
int image_height = 240;

bool verbose = false;

const int ChessBoardSize_w = 7;
const int ChessBoardSize_h = 7;
// Calibration stuff
bool			calibration_done = false;
const CvSize 	ChessBoardSize = cvSize(ChessBoardSize_w,ChessBoardSize_h);
//float 			SquareWidth = 21.6f; //实际距离 毫米单位 在A4纸上为两厘米
float 			SquareWidth = 17; //投影实际距离 毫米单位  200

const   int NPoints = ChessBoardSize_w*ChessBoardSize_h;
const   int NImages = 20; //Number of images to collect

CvPoint2D32f corners[NPoints*NImages];
int corner_count[NImages] = {0};
int captured_frames = 0;

CvMat *intrinsics;
CvMat *distortion_coeff;
CvMat *rotation_vectors;
CvMat *translation_vectors;
CvMat *object_points;
CvMat *point_counts;
CvMat *image_points;
int find_corners_result =0 ;


void on_mouse( int event, int x, int y, int flags, void* param )
{

    if( event == CV_EVENT_LBUTTONDOWN )
    {
		//calibration_done = true;
    }
}


int main(int argc, char *argv[])
{


  CvFont font;
  cvInitFont( &font, CV_FONT_VECTOR0,5, 5, 0, 7, 8);

  intrinsics 		= cvCreateMat(3,3,CV_32FC1);
  distortion_coeff 	= cvCreateMat(1,4,CV_32FC1);
  rotation_vectors 	= cvCreateMat(NImages,3,CV_32FC1);
  translation_vectors 	= cvCreateMat(NImages,3,CV_32FC1);

  point_counts 		= cvCreateMat(NImages,1,CV_32SC1);

  object_points 	= cvCreateMat(NImages*NPoints,3,CV_32FC1);
  image_points 		= cvCreateMat(NImages*NPoints,2,CV_32FC1);


  // Function to fill in the real-world points of the checkerboard
  InitCorners3D(object_points, ChessBoardSize, NImages, SquareWidth);


  CvCapture* capture = 0;


  if( argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0])))
	  capture = cvCaptureFromCAM( argc == 2 ? argv[1][0] - '0' : 0 );
  else if( argc == 2 )
	  capture = cvCaptureFromAVI( argv[1] );

  if( !capture )
  {
	  fprintf(stderr,"Could not initialize capturing...\n");
	  return -1;
  }


  // Initialize all of the IplImage structures
  current_frame_rgb = cvCreateImage(cvSize(image_width, image_height), IPL_DEPTH_8U, 3);

  IplImage *current_frame_rgb2 = cvCreateImage(cvSize(image_width, image_height), IPL_DEPTH_8U, 3);
  current_frame_gray = cvCreateImage(cvSize(image_width, image_height), IPL_DEPTH_8U, 1);

  chessBoard_Img   = cvCreateImage(cvSize(image_width, image_height), IPL_DEPTH_8U, 3);
  current_frame_rgb2->origin = chessBoard_Img->origin  = current_frame_gray->origin = current_frame_rgb->origin = 1;

  makeChessBoard();

 // cvNamedWindow( "result", 0);
 // cvNamedWindow( "Window 0", 0);
 // cvNamedWindow( "grid", 0);
  cvMoveWindow( "grid", 100,100);
  cvSetMouseCallback( "Window 0", on_mouse, 0 );
  cvCreateTrackbar("Thresholdness","Window 0",&Thresholdness, 255,0);

  while (!calibration_done)
  {

	while (captured_frames < NImages)
    {
	  current_frame_rgb = cvQueryFrame( capture );
	  //current_frame_rgb = cvLoadImage( "chess1.jpg" );
	 // cvCopy(chessBoard_Img,current_frame_rgb);

	  if( !current_frame_rgb )
		  break;

	 // cvCopy(current_frame_rgb,current_frame_rgb2);
	 current_frame_rgb2 = cvCloneImage(current_frame_rgb);
     cvCvtColor(current_frame_rgb, current_frame_gray, CV_BGR2GRAY);

	find_corners_result = cvFindChessboardCorners(current_frame_gray,
                                          ChessBoardSize,
                                          &corners[captured_frames*NPoints],
                                          &corner_count[captured_frames],
                                          0);



	cvDrawChessboardCorners(current_frame_rgb2, ChessBoardSize, &corners[captured_frames*NPoints], NPoints, find_corners_result);


	cvShowImage("Window 0",current_frame_rgb2);
	cvShowImage("grid",chessBoard_Img);

	if(find_corners_result==1)
	{
		cvWaitKey(2000);
		cvSaveImage("c:\\hardyinCV.jpg",current_frame_rgb2);
		captured_frames++;
	}
	//cvShowImage("result",current_frame_gray);

	intrinsics->data.fl[0] = 256.8093262;   //fx
	intrinsics->data.fl[2] = 160.2826538;   //cx
	intrinsics->data.fl[4] = 254.7511139;   //fy
	intrinsics->data.fl[5] = 127.6264572;   //cy

	intrinsics->data.fl[1] = 0;
	intrinsics->data.fl[3] = 0;
	intrinsics->data.fl[6] = 0;
	intrinsics->data.fl[7] = 0;
	intrinsics->data.fl[8] = 1;

	distortion_coeff->data.fl[0] = -0.193740;  //k1
	distortion_coeff->data.fl[1] = -0.378588;  //k2
	distortion_coeff->data.fl[2] = 0.028980;   //p1
	distortion_coeff->data.fl[3] = 0.008136;   //p2

	cvWaitKey(40);
	find_corners_result = 0;
    }
	//if (find_corners_result !=0)
	{

		printf("\n");

		cvSetData( image_points, corners, sizeof(CvPoint2D32f));
		cvSetData( point_counts, &corner_count, sizeof(int));


		cvCalibrateCamera2( object_points,
			image_points,
			point_counts,
			cvSize(image_width,image_height),
			intrinsics,
			distortion_coeff,
			rotation_vectors,
			translation_vectors,
			0);


		// [fx 0 cx; 0 fy cy; 0 0 1].
		cvUndistort2(current_frame_rgb,current_frame_rgb,intrinsics,distortion_coeff);
		cvShowImage("result",current_frame_rgb);


		float intr[3][3] = {{0.0}};
		float dist[4] = {0.0};
		float tranv[3] = {0.0};
		float rotv[3] = {0.0};

		for ( int i = 0; i < 3; i++)
		{
			for ( int j = 0; j < 3; j++)
			{
				intr[i][j] = ((float*)(intrinsics->data.ptr + intrinsics->step*i))[j];
			}
			dist[i] = ((float*)(distortion_coeff->data.ptr))[i];
			tranv[i] = ((float*)(translation_vectors->data.ptr))[i];
			rotv[i] = ((float*)(rotation_vectors->data.ptr))[i];
		}
		dist[3] = ((float*)(distortion_coeff->data.ptr))[3];

		printf("-----------------------------------------\n");
		printf("INTRINSIC MATRIX: \n");
		printf("[ %6.4f %6.4f %6.4f ] \n", intr[0][0], intr[0][1], intr[0][2]);
		printf("[ %6.4f %6.4f %6.4f ] \n", intr[1][0], intr[1][1], intr[1][2]);
		printf("[ %6.4f %6.4f %6.4f ] \n", intr[2][0], intr[2][1], intr[2][2]);
		printf("-----------------------------------------\n");
		printf("DISTORTION VECTOR: \n");
		printf("[ %6.4f %6.4f %6.4f %6.4f ] \n", dist[0], dist[1], dist[2], dist[3]);
		printf("-----------------------------------------\n");
		printf("ROTATION VECTOR: \n");
		printf("[ %6.4f %6.4f %6.4f ] \n", rotv[0], rotv[1], rotv[2]);
		printf("TRANSLATION VECTOR: \n");
		printf("[ %6.4f %6.4f %6.4f ] \n", tranv[0], tranv[1], tranv[2]);
		printf("-----------------------------------------\n");

		cvWaitKey(0);

		calibration_done = true;
	}

  }

  exit(0);
  cvDestroyAllWindows();
}

void InitCorners3D(CvMat *Corners3D, CvSize ChessBoardSize, int NImages, float SquareSize)
{
  int CurrentImage = 0;
  int CurrentRow = 0;
  int CurrentColumn = 0;
  int NPoints = ChessBoardSize.height*ChessBoardSize.width;
  float * temppoints = new float[NImages*NPoints*3];

  // for now, assuming we're row-scanning
  for (CurrentImage = 0 ; CurrentImage < NImages ; CurrentImage++)
  {
    for (CurrentRow = 0; CurrentRow < ChessBoardSize.height; CurrentRow++)
    {
      for (CurrentColumn = 0; CurrentColumn < ChessBoardSize.width; CurrentColumn++)
      {
		  temppoints[(CurrentImage*NPoints*3)+(CurrentRow*ChessBoardSize.width + CurrentColumn)*3]=(float)CurrentRow*SquareSize;
		  temppoints[(CurrentImage*NPoints*3)+(CurrentRow*ChessBoardSize.width + CurrentColumn)*3+1]=(float)CurrentColumn*SquareSize;
		  temppoints[(CurrentImage*NPoints*3)+(CurrentRow*ChessBoardSize.width + CurrentColumn)*3+2]=0.f;
      }
    }
  }
  (*Corners3D) = cvMat(NImages*NPoints,3,CV_32FC1, temppoints);
}

int myFindChessboardCorners( const void* image, CvSize pattern_size,
                             CvPoint2D32f* corners, int* corner_count,
                             int flags )

{


	IplImage* eig = cvCreateImage( cvGetSize(image), 32, 1 );
	IplImage* temp = cvCreateImage( cvGetSize(image), 32, 1 );
	double quality = 0.01;
	double min_distance = 5;
	int win_size =10;

	int count = pattern_size.width * pattern_size.height;
	cvGoodFeaturesToTrack( image, eig, temp, corners, &count,
		quality, min_distance, 0, 3, 0, 0.04 );
	cvFindCornerSubPix( image, corners, count,
		cvSize(win_size,win_size), cvSize(-1,-1),
		cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));

	cvReleaseImage( &eig );
	cvReleaseImage( &temp );

	return 1;
}

void makeChessBoard()
{

  CvScalar e;
  e.val[0] =255;
  e.val[1] =255;
  e.val[2] =255;
  cvSet(chessBoard_Img,e,0);
  for(int i = 0;i<ChessBoardSize.width+1;i++)
	  for(int j = 0;j<ChessBoardSize.height+1;j++)
	  {
		  int w =(image_width)/2/(ChessBoardSize.width);
		  int h = w; //(image_height)/2/(ChessBoardSize.height);

		  int ii = i+1;
		 // int iii = ii+1;
		  int jj =j+1;
		  //int jjj =jj+1;
		  int s_x = image_width/6;

		if((i+j)%2==1)
		   cvRectangle( chessBoard_Img, cvPoint(w*i+s_x,h*j+s_x),cvPoint(w*ii-1+s_x,h*jj-1+s_x), CV_RGB(0,0,0),CV_FILLED, 8, 0 );
	  }
}
