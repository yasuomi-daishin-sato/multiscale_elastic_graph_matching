
//#include <iostream>
//using namespace std;

//#include <cv.h>
//#include <highgui.h>

#include "stdafx.h"
#include "_cvgabor.h"

//--------------------------------------------------------------------
//�ݽ�׸�
//--------------------------------------------------------------------
//���Ұ�
//iMu		�Ȗ͗l�̕����p�x�̾�� iMu*PI/8 (����iMu=1�Ȃ�22.5��)
//iNu 		�Ȗ͗l�̕����̾�� �ݒ�\�͈�:-5~��
//dSigma	�޳��֐��̕��U�l��,
//�߂�l:	None
//Create a gabor with a orientation iMu*PI/8, a scale iNu, and a sigma value dSigma. ��Ԏ��g�� (F) is set to sqrt(2) defaultly. It calls Init() to generate parameters and kernels.
//--------------------------------------------------------------------
CvGabor::CvGabor(int iMu, double iNu, double dSigma)
{ 
	F = sqrt(2.0);
	Init(iMu, iNu, dSigma, F);
}

//--------------------------------------------------------------------
//�ݽ�׸�
//--------------------------------------------------------------------
//���Ұ�:
//iMu		�Ȗ͗l�̕����p�x�̾�� iMu*PI/8
//iNu 		�Ȗ͗l�̕����̾�� �ݒ�\�͈�:-5~��
//dSigma 	�޳��֐��̕��U�l��	
//dF		��Ԏ��g��
//�߂�l:	None
//Create a gabor with a orientation iMu*PI/8, a scale iNu, a sigma value dSigma, and a spatial frequence dF. It calls Init() to generate parameters and kernels.
//--------------------------------------------------------------------
CvGabor::CvGabor(int iMu, double iNu, double dSigma, double dF)
{
	Init(iMu, iNu, dSigma, dF);
}
//--------------------------------------------------------------------
//�ݽ�׸�
//--------------------------------------------------------------------
//���Ұ�:
//dPhi		�ʑ��̾��
//iNu 		�Ȗ͗l�̕����̾�� �ݒ�\�͈�:-5~��
//�߂�l:	None
//Create a gabor with a orientation dPhi, and with a scale iNu. The sigma (Sigma) and the spatial frequence (F) are set to 2*PI and sqrt(2) defaultly. It calls Init() to generate parameters and kernels.
//--------------------------------------------------------------------
CvGabor::CvGabor(double dPhi, double iNu)
{
	Sigma = 2*PI;
	F = sqrt(2.0);
	Init(dPhi, iNu, Sigma, F);
}
//--------------------------------------------------------------------
//CvGabor::CvGabor(int iMu, double iNu)
//--------------------------------------------------------------------
CvGabor::CvGabor(int iMu, double iNu)
{
	double dSigma = 2*PI; 
	F = sqrt(2.0);
	Init(iMu, iNu, dSigma, F);
}

//--------------------------------------------------------------------
//�ݽ�׸�
//--------------------------------------------------------------------
//���Ұ�:
//dPhi		�ʑ��̾��
//iNu 		�Ȗ͗l�̕����̾�� �ݒ�\�͈�:-5~��
//dSigma	�޳��֐��̕��U�l��
//�߂�l:	None
//Create a gabor with a orientation dPhi, a scale iNu, and a sigma value dSigma. ��Ԏ��g�� (F) is set to sqrt(2) defaultly. It calls Init() to generate parameters and kernels.
//--------------------------------------------------------------------
CvGabor::CvGabor(double dPhi, double iNu, double dSigma)
{
	F = sqrt(2.0);
	Init(dPhi, iNu, dSigma, F);
}
///--------------------------------------------------------------------
//�ݽ�׸�
//--------------------------------------------------------------------
//���Ұ�:
//dPhi		�ʑ��̾��
//iNu 		�Ȗ͗l�̕����̾�� �ݒ�\�͈�:-5~��
//dSigma 	�޳��֐��̕��U�l��
//dF		��Ԏ��g��
//�߂�l:	None
//Create a gabor with a orientation dPhi, a scale iNu, a sigma value dSigma, and a spatial frequence dF. It calls Init() to generate parameters and kernels.
//--------------------------------------------------------------------
CvGabor::CvGabor(double dPhi, double iNu, double dSigma, double dF)
{
	Init(dPhi, iNu, dSigma,dF);
}
//--------------------------------------------------------------------
//�޽�׸�
//--------------------------------------------------------------------
CvGabor::~CvGabor()
{
	cvReleaseMat( &Real );
	cvReleaseMat( &Imag );
}
//--------------------------------------------------------------------
//ҿ���
//--------------------------------------------------------------------
//CvGabor::IsInit()
//���ްٸ׽�̕K�v�ϐ��iF, K, Kmax, Phi, Sigma�j������������Ă��邩����
//���Ұ�:	None
//�߂�l:	bool�ϐ�, TRUE is initilised or FALSE is non-initilised.
//--------------------------------------------------------------------
bool CvGabor::IsInit()
{
	return bInitialised;
}
//--------------------------------------------------------------------
//CvGabor::mask_width()
//̨�����擾 Sigma ��iNu�̒l�Ō��肳���
//���Ұ�:	None
//�߂�l:	long�^��̨������Ԃ�
//--------------------------------------------------------------------
long CvGabor::mask_width()
{
	long lWidth;
	if (IsInit() == false)  {
		perror ("Error: The Object has not been initilised in mask_width()!\n");
		return 0;
	}
	else {
		//determine the width of Mask
		double dModSigma = Sigma/K;
		double dWidth = cvRound(dModSigma*6 + 1);
		//test whether dWidth is an odd.
		if (fmod(dWidth, 2.0)==0.0) dWidth++;
		lWidth = (long)dWidth;

		return lWidth;
	}
}
//--------------------------------------------------------------------
//CvGabor::creat_kernel()
//���ް�̨���̍쐬 - REAL and IMAG, with an orientation and a scale 
//���Ұ�:	None
//�߂�l:	None
//--------------------------------------------------------------------
void CvGabor::creat_kernel()
{
	if (IsInit() == false) {perror("Error: The Object has not been initilised in creat_kernel()!\n");}
	else {
		CvMat *mReal, *mImag;
		mReal = cvCreateMat( Width, Width, CV_32FC1);
		mImag = cvCreateMat( Width, Width, CV_32FC1);

		/**************************** Gabor Function ****************************/ 
		int x, y;
		double dReal;
		double dImag;
		double dTemp1, dTemp2, dTemp3;

		for (int i = 0; i < Width; i++)
		{
			for (int j = 0; j < Width; j++)
			{
				x = i-(Width-1)/2;
				y = j-(Width-1)/2;
				dTemp1 = (pow(K,2)/pow(Sigma,2))*exp(-(pow((double)x,2)+pow((double)y,2))*pow(K,2)/(2*pow(Sigma,2)));
				dTemp2 = cos(K*cos(Phi)*x + K*sin(Phi)*y) - exp(-(pow(Sigma,2)/2));
				dTemp3 = sin(K*cos(Phi)*x + K*sin(Phi)*y);
				dReal = dTemp1*dTemp2;
				dImag = dTemp1*dTemp3; 
				//gan_mat_set_el(pmReal, i, j, dReal);
				//cvmSet( (CvMat*)mReal, i, j, dReal );
				cvSetReal2D((CvMat*)mReal, i, j, dReal );
				//gan_mat_set_el(pmImag, i, j, dImag);
				//cvmSet( (CvMat*)mImag, i, j, dImag );
				cvSetReal2D((CvMat*)mImag, i, j, dImag );
			} 
		}

		/**************************** Gabor Function ****************************/
		bKernel = true;
		cvCopy(mReal, Real, NULL);
		cvCopy(mImag, Imag, NULL);
		//printf("A %d x %d Gabor kernel with %f PI in arc is created.\n", Width, Width, Phi/PI);
		cvReleaseMat( &mReal );
		cvReleaseMat( &mImag );
	}
}
//--------------------------------------------------------------------
//CvGabor::get_image(int Type)
//�w�肵�����ް�̨��������("REAL","IMAG","MAG","PHASE")�̉摜���擾
//���Ұ�:	Type�@���ް�̨��������, REAL:����, IMAG:����, MAG:�U��, PHASE:�ʑ�   
//�߂�l:	IplImage�^�̉摜�ް�
//--------------------------------------------------------------------
IplImage* CvGabor::get_image(int Type)
{
	if(IsKernelCreate() == false)
	{ 
		perror("Error: the Gabor kernel has not been created in get_image()!\n");
		return NULL;
	}
	else
	{  
		IplImage* pImage;
		IplImage *newimage;
		newimage = cvCreateImage(cvSize(Width,Width), IPL_DEPTH_8U, 1 );
		//printf("Width is %d.\n",(int)Width);
		//printf("Sigma is %f.\n", Sigma);
		//printf("F is %f.\n", F);
		//printf("Phi is %f.\n", Phi);

		//pImage = gan_image_alloc_gl_d(Width, Width);
		pImage = cvCreateImage( cvSize(Width,Width), IPL_DEPTH_32F, 1 );


		CvMat* kernel = cvCreateMat(Width, Width, CV_32FC1);
		double ve;
		CvSize size = cvGetSize( kernel );
		int rows = size.height;
		int cols = size.width;
		switch(Type)
		{
		case 1:  //����

			cvCopy( (CvMat*)Real, (CvMat*)kernel, NULL );
			//pImage = cvGetImage( (CvMat*)kernel, pImageGL );
			for (int i = 0; i < rows; i++)
			{
				for (int j = 0; j < cols; j++)
				{
					ve = cvGetReal2D((CvMat*)kernel, i, j);
					cvSetReal2D( (IplImage*)pImage, j, i, ve );
				}
			}
			break;
		case 2:  //����
			cvCopy( (CvMat*)Imag, (CvMat*)kernel, NULL );
			//pImage = cvGetImage( (CvMat*)kernel, pImageGL );
			for (int i = 0; i < rows; i++)
			{
				for (int j = 0; j < cols; j++)
				{
					ve = cvGetReal2D((CvMat*)kernel, i, j);
					cvSetReal2D( (IplImage*)pImage, j, i, ve );
				}
			}
			break; 
		case 3:  //Magnitude
			///@todo  
			break;
		case 4:  //Phase
			///@todo
			break;
		}
		cvNormalize((IplImage*)pImage, (IplImage*)pImage, 0, 255, CV_MINMAX, NULL );

		cvConvertScaleAbs( (IplImage*)pImage, (IplImage*)newimage, 1, 0 );

		cvReleaseMat(&kernel);

		cvReleaseImage(&pImage);

		return newimage;
	}
}
//--------------------------------------------------------------------
//CvGabor::IsKernelCreate()
//���ް�̨�����쐬����Ă��邩���Ȃ�������
//���Ұ�:	None
//�߂�l:	bool�ϐ�, TRUE is created or FALSE is non-created.
//--------------------------------------------------------------------
bool CvGabor::IsKernelCreate()
{
	return bKernel;
}
//--------------------------------------------------------------------
//CvGabor::get_mask_width()
//���ް�̨���̉������擾
//���Ұ�: None
//�߂�l: Pointer to long type width of mask.
//--------------------------------------------------------------------
long CvGabor::get_mask_width()
{
	return Width;
}
//--------------------------------------------------------------------
//CvGabor::Init(int iMu, double iNu, double dSigma, double dF)
//the orientation iMu, the scale iNu, the sigma dSigma, the frequency dF�Ŷ��ްٸ׽�̏�����
//���̏������s�������creat_kernel()�����s
//���Ұ�:
//iMu 		�Ȗ͗l�̕����p�x�̾�� iMu*PI/8 (����iMu=1�Ȃ�22.5��)
//iNu 		�Ȗ͗l�̕����̾�� �ݒ�\�͈�:-5~��
//dSigma 	�޳��֐��̕��U�l��, �ʏ�: 2*PI
//dF 		��Ԏ��g�� , �ʏ�:sqrt(2)
//�߂�l:	NULL
//--------------------------------------------------------------------
void CvGabor::Init(int iMu, double iNu, double dSigma, double dF)
{
	//Initilise the parameters 
	bInitialised = false;
	bKernel = false;

	Sigma = dSigma;	//�޳��֐��̕��U�l��
	F = dF;			//��Ԏ��g��
	Kmax = PI/2;	//�Ȗ͗l�̕����ő�l

	// Absolute value of K
	K = Kmax / pow(F, (double)iNu);	
	Phi = PI*iMu/8;			//�ʑ��̾��
	bInitialised = true;	//�����������׸�
	Width = mask_width();	//̨������
	Real = cvCreateMat( Width, Width, CV_32FC1);
	Imag = cvCreateMat( Width, Width, CV_32FC1);
	creat_kernel();  
}
//--------------------------------------------------------------------
//CvGabor::Init(double dPhi, double iNu, double dSigma, double dF)
//the orientation dPhi, the scale iNu, the sigma dSigma, the frequency dF�Ŷ��ްٸ׽��������
//���̏������s�������creat_kernel()�����s
//���Ұ�:
//dPhi 		�ʑ��̾��
//iNu 		�Ȗ͗l�̕����̾�� �ݒ�\�͈�:-5~��
//dSigma 	�޳��֐��̕��U�l��, �ʏ�: 2*PI
//dF 		��Ԏ��g�� , �ʏ�:sqrt(2)
//�߂�l:	None
//--------------------------------------------------------------------
void CvGabor::Init(double dPhi, double iNu, double dSigma, double dF)
{
	bInitialised = false;
	bKernel = false;
	Sigma = dSigma;
	F = dF;

	Kmax = PI/2;	

	// Absolute value of K
	K = Kmax / pow(F, (double)iNu);
	Phi = dPhi;
	bInitialised = true;
	Width = mask_width();
	Real = cvCreateMat( Width, Width, CV_32FC1);
	Imag = cvCreateMat( Width, Width, CV_32FC1);
	creat_kernel();  
}
//--------------------------------------------------------------------
//CvGabor::get_matrix(int Type)
//���Ұ�:	Type	���ް�̨��������, REAL:����, IMAG:����, MAG:�U��, PHASE:�ʑ� 
//�߂�l:   ���ް�̨����CvMat�^�߲���@�װ�Ȃ�NULL
//CvMat�^�̶��ް�̨���擾
//--------------------------------------------------------------------
CvMat* CvGabor::get_matrix(int Type)
{
	if (!IsKernelCreate()) {perror("Error: the ���ް�̨�� has not been created!\n"); return NULL;}
	switch (Type)
	{
	case CV_GABOR_REAL:
		return Real;
		break;
	case CV_GABOR_IMAG:
		return Imag;
		break;
	case CV_GABOR_MAG:
		return NULL;
		break;
	case CV_GABOR_PHASE:
		return NULL;
		break;
	default:
		return NULL;
	}
}
//--------------------------------------------------------------------
//CvGabor::output_file(const char *filename, Gan_ImageFileFormat file_format, int Type)
//���ް�̨�����摜�Ƃ��ď�����
//���Ұ�:
//filename	�ۑ��摜��̧�ٖ�
//Type		���ް�̨��������, REAL:����, IMAG:����, MAG:�U��, PHASE:�ʑ�  
//�߂�l:	None
//Writes an image from the provided image structure into the given file and the type of ���ް�̨��.
//--------------------------------------------------------------------
void CvGabor::output_file(const char *filename, int Type)
{
	IplImage *pImage;
	pImage = get_image(Type);
	if(pImage != NULL)
	{
		if( cvSaveImage(filename, pImage )) printf("%s has been written successfully!\n", filename);
		else printf("Error: writting %s has failed!\n", filename);
	}
	else 
		perror("Error: the image is empty in output_file()!\n"); 

	cvReleaseImage(&pImage);
}
//--------------------------------------------------------------------
//CvGabor::show(int Type)
//���ް�̨����GUI�\��
//���Ұ��@Type	���ް�̨��������, REAL:����, IMAG:����, MAG:�U��, PHASE:�ʑ�  
//--------------------------------------------------------------------
void CvGabor::show(int Type)
{
	if(!IsInit()) {
		perror("Error: the ���ް�̨�� has not been created!\n");
	}
	else {
		//IplImage *pImage;
		//pImage = get_image(Type);
		//cvNamedWindow("Testing",1);
		//cvShowImage("Testing",pImage);
		//cvWaitKey(0);
		//cvDestroyWindow("Testing");
		//cvReleaseImage(&pImage);
	}
}
//--------------------------------------------------------------------
//CvGabor::normalize( const CvArr* src, CvArr* dst, double a, double b, int norm_type, const CvArr* mask )
//���K��
//--------------------------------------------------------------------
void CvGabor::normalize( const CvArr* src, CvArr* dst, double a, double b, int norm_type, const CvArr* mask )
{
	CvMat* tmp = 0;
//	__BEGIN__;

	double scale, shift;

	if( norm_type == CV_MINMAX )
	{
		double smin = 0, smax = 0;
		double dmin = MIN( a, b ), dmax = MAX( a, b );
		cvMinMaxLoc( src, &smin, &smax, 0, 0, mask );
		scale = (dmax - dmin)*(smax - smin > DBL_EPSILON ? 1./(smax - smin) : 0);
		shift = dmin - smin*scale;
	}
	else if( norm_type == CV_L2 || norm_type == CV_L1 || norm_type == CV_C )
	{
		CvMat *s = (CvMat*)src, *d = (CvMat*)dst;

		scale = cvNorm( src, 0, norm_type, mask );
		scale = scale > DBL_EPSILON ? 1./scale : 0.;
		shift = 0;
	}
	else {}

	if( !mask )
		cvConvertScale( src, dst, scale, shift );
	else
	{
		cvConvertScale( src, tmp, scale, shift );
		cvCopy( tmp, dst, mask );
	}

//	__END__;

	if( tmp )
		cvReleaseMat( &tmp );
}
//--------------------------------------------------------------------
//CvGabor::conv_img_a(IplImage *src, IplImage *dst, int Type)
//�􍞂ݐϕ�
//���Ұ�:
//src	���͉摜
//dst	�o�͉摜  
//Type	���ް�̨��������, REAL:����, IMAG:����, MAG:�U��, PHASE:�ʑ�  
//�߂�l:	None
//--------------------------------------------------------------------

void CvGabor::conv_img_a(IplImage *src, IplImage *dst, int Type)
{
	double ve,re,im;
	int width = src->width;
	int height = src->height;
	CvMat *mat = cvCreateMat(src->width, src->height, CV_32FC1);

	for (int i = 0; i < width; i++){
		for (int j = 0; j < height; j++){
			ve = cvGetReal2D((IplImage*)src, j, i);
			cvSetReal2D( (CvMat*)mat, i, j, ve );
		}
	}
	CvMat *rmat = cvCreateMat(width, height, CV_32FC1);
	CvMat *imat = cvCreateMat(width, height, CV_32FC1);
	CvMat *kernel = cvCreateMat( Width, Width, CV_32FC1 );

	switch (Type)
	{
	case CV_GABOR_REAL:
		cvCopy( (CvMat*)Real, (CvMat*)kernel, NULL );
		cvFilter2D( (CvMat*)mat, (CvMat*)mat, (CvMat*)kernel, cvPoint( (Width-1)/2, (Width-1)/2));
		break;
	case CV_GABOR_IMAG:
		cvCopy( (CvMat*)Imag, (CvMat*)kernel, NULL );
		cvFilter2D( (CvMat*)mat, (CvMat*)mat, (CvMat*)kernel, cvPoint( (Width-1)/2, (Width-1)/2));
		break;
	case CV_GABOR_MAG:
		/* Real Response */
		cvCopy( (CvMat*)Real, (CvMat*)kernel, NULL );
		cvFilter2D( (CvMat*)mat, (CvMat*)rmat, (CvMat*)kernel, cvPoint( (Width-1)/2, (Width-1)/2));
		/* Imag Response */
		cvCopy( (CvMat*)Imag, (CvMat*)kernel, NULL );
		cvFilter2D( (CvMat*)mat, (CvMat*)imat, (CvMat*)kernel, cvPoint( (Width-1)/2, (Width-1)/2));
		/* Magnitude response is the square root of the sum of the square of real response and imaginary response */
		for (int i = 0; i < width; i++)
		{
			for (int j = 0; j < height; j++)
			{
				re = cvGetReal2D((CvMat*)rmat, i, j);
				im = cvGetReal2D((CvMat*)imat, i, j);
				ve = sqrt(re*re + im*im);
				cvSetReal2D( (CvMat*)mat, i, j, ve );
			}
		}       
		break;
	case CV_GABOR_PHASE:
		break;
	}

	if (dst->depth == IPL_DEPTH_8U){
		cvNormalize((CvMat*)mat, (CvMat*)mat, 0, 255, CV_MINMAX, NULL);
		for (int i = 0; i < width; i++){
			for (int j = 0; j < height; j++){
				ve = cvGetReal2D((CvMat*)mat, i, j);
				ve = cvRound(ve);
				cvSetReal2D( (IplImage*)dst, j, i, ve );
			}
		}
	}

	if (dst->depth == IPL_DEPTH_32F){
		for (int i = 0; i < width; i++){
			for (int j = 0; j < height; j++){
				ve = cvGetReal2D((CvMat*)mat, i, j);
				cvSetReal2D( (IplImage*)dst, j, i, ve );
			}
		}
	} 

	cvReleaseMat(&kernel);
	cvReleaseMat(&imat);
	cvReleaseMat(&rmat);
	cvReleaseMat(&mat);
}
//--------------------------------------------------------------------
//CvGabor::conv_img(IplImage *src, IplImage *dst, int Type)
//�􍞂ݐϕ�
//���Ұ�:
//src	���͉摜
//dst	�o�͉摜  
//Type	���ް�̨��������, REAL:����, IMAG:����, MAG:�U��, PHASE:�ʑ�  
//�߂�l:	None
//--------------------------------------------------------------------
void CvGabor::conv_img(IplImage *src, IplImage *dst, int Type)
{
	double ve;
	CvMat *mat = cvCreateMat(src->width, src->height, CV_32FC1);
	for (int i = 0; i < src->width; i++){
		for (int j = 0; j < src->height; j++){
			ve = CV_IMAGE_ELEM(src, uchar, j, i);
			CV_MAT_ELEM(*mat, float, i, j) = (float)ve;
		}
	}
	CvMat *rmat = cvCreateMat(src->width, src->height, CV_32FC1);
	CvMat *imat = cvCreateMat(src->width, src->height, CV_32FC1);

	switch (Type)
	{
	case CV_GABOR_REAL:
		cvFilter2D( (CvMat*)mat, (CvMat*)mat, (CvMat*)Real, cvPoint( (Width-1)/2, (Width-1)/2));
		break;
	case CV_GABOR_IMAG:
		cvFilter2D( (CvMat*)mat, (CvMat*)mat, (CvMat*)Imag, cvPoint( (Width-1)/2, (Width-1)/2));
		break;
	case CV_GABOR_MAG:
		cvFilter2D( (CvMat*)mat, (CvMat*)rmat, (CvMat*)Real, cvPoint( (Width-1)/2, (Width-1)/2));
		cvFilter2D( (CvMat*)mat, (CvMat*)imat, (CvMat*)Imag, cvPoint( (Width-1)/2, (Width-1)/2));

		cvPow(rmat,rmat,2); 
		cvPow(imat,imat,2);
		cvAdd(imat,rmat,mat); 
		cvPow(mat,mat,0.5); 
		break;
	case CV_GABOR_PHASE:
		break;
	}

	if (dst->depth == IPL_DEPTH_8U){
		cvNormalize((CvMat*)mat, (CvMat*)mat, 0, 255, CV_MINMAX);
		for (int i = 0; i < mat->rows; i++){
			for (int j = 0; j < mat->cols; j++){
				ve = CV_MAT_ELEM(*mat, float, i, j);
				CV_IMAGE_ELEM(dst, uchar, j, i) = (uchar)cvRound(ve);
			}
		}
	}

	if (dst->depth == IPL_DEPTH_32F){
		for (int i = 0; i < mat->rows; i++){
			for (int j = 0; j < mat->cols; j++){
				ve = cvGetReal2D((CvMat*)mat, i, j);
				cvSetReal2D( (IplImage*)dst, j, i, ve );
			}
		}
	}
	cvReleaseMat(&imat);
	cvReleaseMat(&rmat);
	cvReleaseMat(&mat);
}

void conv_element(CvMat* mat, CvMat* out, CvMat* k, int *px, int *py, int len)
{
	float v;
	int dr, dc;
	int r, c;
	int x, y;
	for (int p = 0; p < len; p ++) {
		y = px[p];//swap
		x = py[p];
//	for (int y = 0; y < mat->rows; y++){
//		for (int x = 0; x < mat->cols; x++){
			v = 0;
			dr = y - k->rows / 2;
			dc = x - k->cols / 2;
			for (int i = 0; i < k->rows; i++){
				r = dr + i;
				if (r < 0) r = 0;
				if (r >= mat->rows) r = mat->rows-1;
				for (int j = 0; j < k->cols; j++){
					c = dc + j;
					if (c < 0) c = 0;
					if (c >= mat->cols) c = mat->cols-1;
					v += CV_MAT_ELEM(*mat, float, r, c) * CV_MAT_ELEM(*k, float, i, j);
				}
			}
			CV_MAT_ELEM(*out, float, y, x) = v;
//		}
//	}
	}
}

void CvGabor::conv_img_pt(IplImage *src, IplImage *dst, int Type, int*x, int*y, int len)
{
	double ve;
	CvMat *mat = cvCreateMat(src->width, src->height, CV_32FC1);
	for (int i = 0; i < src->width; i++){
		for (int j = 0; j < src->height; j++){
			ve = CV_IMAGE_ELEM(src, uchar, j, i);
			CV_MAT_ELEM(*mat, float, i, j) = (float)ve;
		}
	}
	CvMat *mat2 = cvCreateMat(src->width, src->height, CV_32FC1);
	cvSetZero(mat2); 
	CvMat *rmat = cvCreateMat(src->width, src->height, CV_32FC1);
	CvMat *imat = cvCreateMat(src->width, src->height, CV_32FC1);

	switch (Type)
	{
	case CV_GABOR_REAL:
		conv_element(mat, mat2, Real, x, y, len);
		break;
	case CV_GABOR_IMAG:
		conv_element(mat, mat2, Real, x, y, len);
		break;
	case CV_GABOR_MAG:
		conv_element(mat, rmat, Real, x, y, len);
		conv_element(mat, imat, Imag, x, y, len);

		int px, py;
		float v;
		for (int p = 0; p < len; p ++) {
			py = x[p];//swap
			px = y[p];
			v = CV_MAT_ELEM(*rmat, float, py, px) * CV_MAT_ELEM(*rmat, float, py, px);
			v+= CV_MAT_ELEM(*imat, float, py, px) * CV_MAT_ELEM(*imat, float, py, px);
			CV_MAT_ELEM(*mat2, float, py, px) = sqrtf(v);
		}
		//cvPow(rmat,rmat,2); 
		//cvPow(imat,imat,2);
		//cvAdd(imat,rmat,mat); 
		//cvPow(mat,mat,0.5); 
		break;
	case CV_GABOR_PHASE:
		break;
	}

	cvReleaseMat(&mat);
	mat = mat2;
	mat2 = NULL;

	if (dst->depth == IPL_DEPTH_8U){
		cvNormalize((CvMat*)mat, (CvMat*)mat, 0, 255, CV_MINMAX);
		for (int i = 0; i < mat->rows; i++){
			for (int j = 0; j < mat->cols; j++){
				ve = CV_MAT_ELEM(*mat, float, i, j);
				CV_IMAGE_ELEM(dst, uchar, j, i) = (uchar)cvRound(ve);
			}
		}
	}

	if (dst->depth == IPL_DEPTH_32F){
		for (int i = 0; i < mat->rows; i++){
			for (int j = 0; j < mat->cols; j++){
				ve = cvGetReal2D((CvMat*)mat, i, j);
				cvSetReal2D( (IplImage*)dst, j, i, ve );
			}
		}
	}
	cvReleaseMat(&imat);
	cvReleaseMat(&rmat);
	cvReleaseMat(&mat);
}
