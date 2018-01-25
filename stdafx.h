// stdafx.h : �W���̃V�X�e�� �C���N���[�h �t�@�C���̃C���N���[�h �t�@�C���A�܂���
// �Q�Ɖ񐔂������A�����܂�ύX����Ȃ��A�v���W�F�N�g��p�̃C���N���[�h �t�@�C��
// ���L�q���܂��B
//

#pragma once

//#ifdef _VC2010
//#include "targetver.h"
//#endif

// TODO: �v���O�����ɕK�v�Ȓǉ��w�b�_�[�������ŎQ�Ƃ��Ă��������B

#include <windows.h>
//#include <stdio.h>
#include <tchar.h>
#include <process.h>

#include <cv.h>
#include <highgui.h>

#if CV_MAJOR_VERSION == 2 && CV_MINOR_VERSION == 1
#pragma comment(lib, "cxcore210.lib")
#pragma comment(lib, "cv210.lib")
#pragma comment(lib, "highgui210.lib")
#else
//#pragma comment(lib, "cxcore.lib")
//#pragma comment(lib, "cv.lib")
//#pragma comment(lib, "highgui.lib")
#ifdef _DEBUG
    //Debug���[�h�̏ꍇ
    #pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Debug\\cxcore200d.lib")
    //#pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Debug\\cvhaartraining.lib") 
    //#pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Debug\\cxts200d.lib") 
    //#pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Debug\\ml200d.lib")
    //#pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Debug\\opencv_ffmpeg200d.lib")    
    #pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Debug\\cv200d.lib")  
    #pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Debug\\cvaux200d.lib") 
    #pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Debug\\highgui200d.lib") 
#else
    //Release���[�h�̏ꍇ
    #pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Release\\cxcore200.lib") 
    //#pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Release\\cvhaartraining.lib") 
    //#pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Release\\cxts200.lib")
    //#pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Release\\ml200.lib")
    //#pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Release\\opencv_ffmpeg200.lib")
    #pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Release\\cv200.lib") 
    #pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Release\\cvaux200.lib") 
    #pragma comment(lib,"C:\\OpenCV2.0\\build\\lib\\Release\\highgui200.lib") 
#endif


#endif

#define _USE_MATH_DEFINES
#include <math.h>

#include <MMSystem.h>
#pragma comment(lib, "winmm.lib")

#define RELEASE_ARRAY(x) if(x){delete [] x;x=NULL;}
#define RELEASE_IPLIMAGE(x) if(x){cvReleaseImage(&x);x=NULL;}
#define RELEASE_IPL_ARRAY(x,cnt) {for(int i=0;i<cnt;i++)if(x[i])cvReleaseImage(x+i);delete [] x;x=NULL;}
