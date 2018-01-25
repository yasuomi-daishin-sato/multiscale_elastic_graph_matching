// 比較用アダブースト顔検出

#include "stdafx.h"
#include "ocvfd.h"

#define WN_VIEW "View"

#define CASCADE_PATH "C:\\OpenCV2.1\\data\\haarcascades\\haarcascade_frontalface_default.xml"

int OCVFD::fdbatch(int display)
{
	char str_tmp[1024];
	int *eye_lx = new int[1521];
	int *eye_ly = new int[1521];
	int *eye_rx = new int[1521];
	int *eye_ry = new int[1521];

	int cnt_success=0;

	//正解データ読み込み
	for (int i = 0; i < 1521; i++) {
		char str1[16];
		int  lx, ly, rx, ry;
		FILE *fp=NULL;
		sprintf_s(str_tmp, 1024, "c:\\project\\common\\BioID-FaceDatabase-V1.2\\BioID_%04d.eye", i);
		fopen_s(&fp, str_tmp, "rt");
		if (fp == NULL)
			return 0;
		fscanf_s(fp, "%s", str1, 16);
		fscanf_s(fp, "%s", str1, 16);
		fscanf_s(fp, "%s", str1, 16);
		fscanf_s(fp, "%s", str1, 16);
		fscanf_s(fp, "%d", &lx);
		fscanf_s(fp, "%d", &ly);
		fscanf_s(fp, "%d", &rx);
		fscanf_s(fp, "%d", &ry);
		eye_lx[i] = lx;
		eye_ly[i] = ly;
		eye_rx[i] = rx;
		eye_ry[i] = ry;
		fclose(fp);
	}

	//顔検出準備
	const char *cascade_name = CASCADE_PATH;
	CvHaarClassifierCascade *cascade = 0;
	CvMemStorage *storage = 0;
	CvSeq *faces;
	storage = cvCreateMemStorage (0);
	cascade = (CvHaarClassifierCascade *) cvLoad (cascade_name, 0, 0, 0);
	if (cascade==NULL)
		return 0;

	cvNamedWindow (WN_VIEW, 1);

	IplImage *img_cap=NULL, *img_in=NULL;
	unsigned long time_start = timeGetTime();

	printf("success/progress/images\n");
	
	for (int i = 0; i < 1521; i++) {
		
		sprintf_s(str_tmp, 1024, "c:\\project\\common\\BioID-FaceDatabase-V1.2\\BioID_%04d.pgm", i);
		img_cap = cvLoadImage(str_tmp);

		if (img_cap==NULL)
			continue;

		//-フォーマット変換
		if (img_cap->nChannels == 3) {
			img_in = cvCreateImage(cvSize(img_cap->width, img_cap->height), IPL_DEPTH_8U, 1);
			cvCvtColor(img_cap, img_in, CV_BGR2GRAY);
		} else {
			img_in = cvCloneImage(img_cap);
		}
		cvEqualizeHist (img_in, img_in);
		
		// 顔検出
		cvClearMemStorage (storage);
		faces = cvHaarDetectObjects (img_in, cascade, storage, 1.11, 4, 0, cvSize (60, 60));

		int flag=FALSE;

		for (int j = 0; j < (faces ? faces->total : 0); j++) {
			CvRect *r = (CvRect *) cvGetSeqElem (faces, j);
			if (r->x < eye_lx[i] && r->x+r->width > eye_lx[i] && r->y < eye_ly[i] && r->y+r->height*3/5 > eye_ly[i]
			&&  r->x < eye_rx[i] && r->x+r->width > eye_rx[i] && r->y < eye_ry[i] && r->y+r->height*3/5 > eye_ry[i]) {
				if (display>=1) {
					cvDrawRect(img_in, cvPoint(r->x, r->y), cvPoint(r->x + r->width, r->y + r->height), CV_RGB(255,255,255));
				}
				flag=TRUE;
				cnt_success ++;
				break;
			}
		}
		printf("\r%4d/%4d/1521 %3d%%", cnt_success, i+1, cnt_success*100/(i+1));

		if (display>=1) {
			cvShowImage(WN_VIEW, img_in);
			if (display==1)
				cvWaitKey(1);
			else if(flag==FALSE)
				cvWaitKey(0);
			else
				cvWaitKey(100);
		}

		cvReleaseImage(&img_cap);
		img_cap = NULL;
		cvReleaseImage(&img_in);
	}

	unsigned long time_delta = timeGetTime() - time_start;
	printf("\nTime:%d[ms]\n%d%% %d[ms]", time_delta, cnt_success*100/1521, time_delta/1521);

	delete [] eye_lx;
	delete [] eye_ly;
	delete [] eye_rx;
	delete [] eye_ry;
	cvReleaseMemStorage (&storage);

	cvWaitKey (0);
	cvDestroyAllWindows();

	return 0;
}

int OCVFD::fdloop()
{
	//顔検出準備
	const char *cascade_name = CASCADE_PATH;
	CvHaarClassifierCascade *cascade = 0;
	CvMemStorage *storage = 0;
	CvSeq *faces;
	storage = cvCreateMemStorage (0);
	cascade = (CvHaarClassifierCascade *) cvLoad (cascade_name, 0, 0, 0);
	if (cascade==NULL)
		return 0;

	//カメラ入力準備
	CvCapture *cap=NULL;
	cap = cvCreateCameraCapture(0);
	cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH, 320);
	cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT, 240);

	int key=0;
	IplImage *img_cap=NULL, *img_in=NULL;
	char str_tmp[1024];
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);

	cvNamedWindow(WN_VIEW, 1);
	
	unsigned long time_start = timeGetTime(), time_now, time_delta;

	do {
		img_cap = cvQueryFrame(cap);
		//-フォーマット変換
		if (img_cap->nChannels == 3) {
			img_in = cvCreateImage(cvSize(img_cap->width, img_cap->height), IPL_DEPTH_8U, 1);
			cvCvtColor(img_cap, img_in, CV_BGR2GRAY);
		} else {
			img_in = cvCloneImage(img_cap);
		}
		cvEqualizeHist (img_in, img_in);
		
		// 顔検出
		cvClearMemStorage (storage);
		faces = cvHaarDetectObjects (img_in, cascade, storage, 1.11, 4, 0, cvSize (60, 60));

		time_now = timeGetTime();
		time_delta = time_now - time_start;
		time_start = time_now;

		for (int i = 0; i < (faces ? faces->total : 0); i++) {
			CvRect *r = (CvRect *) cvGetSeqElem (faces, i);
			cvDrawRect(img_in, cvPoint(r->x, r->y), cvPoint(r->x + r->width, r->y + r->height), CV_RGB(255,255,255));
		}

		sprintf_s(str_tmp, 1024, "%dms", time_delta);
		cvPutText(img_in, str_tmp, cvPoint(4, 16+4), &font, CV_RGB(255,255,255)); 
		
		cvShowImage(WN_VIEW, img_in);

		key = cvWaitKey(2);

		//キー入力処理
		//-C:記憶顔更新
		if (key == 'c' && faces) {
			for (int i = 0; i < faces->total; i++) {
				CvRect *r = (CvRect *) cvGetSeqElem (faces, i);
				IplImage *imgtmp = cvCreateImage(cvSize(r->width, r->height), img_in->depth, img_in->nChannels);
				char strtmp[MAX_PATH];
				cvSetImageROI(img_in, *r);
				cvCopyImage(img_in, imgtmp);
				cvResetImageROI(img_in);
				
				sprintf_s(strtmp, MAX_PATH, "data\\img%d_%d.bmp", GetTickCount(), i+1);
				cvSaveImage(strtmp, imgtmp);
				cvReleaseImage(&imgtmp);
			}
		}

		cvReleaseImage(&img_in);
	} while ( key != '\x1b' );

	cvReleaseCapture(&cap);
	cvReleaseMemStorage (&storage);

	// ウィンドウ解放
	cvDestroyAllWindows();
/*
	// (1)画像を読み込む
	if (argc < 2 || (src_img = cvLoadImage (argv[1], CV_LOAD_IMAGE_COLOR)) == 0)    return -1;
	src_gray = cvCreateImage (cvGetSize (src_img), IPL_DEPTH_8U, 1);

	// (2)ブーストされた分類器のカスケードを読み込む
	cascade = (CvHaarClassifierCascade *) cvLoad (cascade_name, 0, 0, 0);

	// (3)メモリを確保し，読み込んだ画像のグレースケール化，ヒストグラムの均一化を行う
	storage = cvCreateMemStorage (0);
	cvClearMemStorage (storage);
	cvCvtColor (src_img, src_gray, CV_BGR2GRAY);
	cvEqualizeHist (src_gray, src_gray);

	// (4)物体（顔）検出
	faces = cvHaarDetectObjects (src_gray, cascade, storage, 1.11, 4, 0, cvSize (40, 40));

	// (5)検出された全ての顔位置に，円を描画する
	for (i = 0; i < (faces ? faces->total : 0); i++) {
		CvRect *r = (CvRect *) cvGetSeqElem (faces, i);
		CvPoint center;
		int radius;
		center.x = cvRound (r->x + r->width * 0.5);
		center.y = cvRound (r->y + r->height * 0.5);
		radius = cvRound ((r->width + r->height) * 0.25);
		cvCircle (src_img, center, radius, colors[i % 8], 3, 8, 0);
	}

	// (6)画像を表示，キーが押されたときに終了
	cvNamedWindow ("Face Detection", CV_WINDOW_AUTOSIZE);
	cvShowImage ("Face Detection", src_img);
	cvWaitKey (0);

	cvDestroyWindow ("Face Detection");
	cvReleaseImage (&src_img);
	cvReleaseImage (&src_gray);
	cvReleaseMemStorage (&storage);
*/	return 0;
}

