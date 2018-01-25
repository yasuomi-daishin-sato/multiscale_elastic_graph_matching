/***************************************************************************
*   Copyright (C) 2006 by Mian Zhou   *   modefer : k.h. 2008
*   M.Zhou@reading.ac.uk   *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
*   HP:http://www.personal.rdg.ac.uk/~sir02mz/CGabor/example.html
***************************************************************************/

#ifndef CVGABOR_H
#define CVGABOR_H

//--------------------------------------------------------------------
//��`�l
//--------------------------------------------------------------------
#define PI 3.14159265
#define CV_GABOR_REAL 1
#define CV_GABOR_IMAG 2
#define CV_GABOR_MAG  3
#define CV_GABOR_PHASE 4

//--------------------------------------------------------------------
//���ްٸ׽
//--------------------------------------------------------------------
class CvGabor{
public:
	//�ݽ�׸�
	CvGabor(){};
	CvGabor(int iMu, double iNu, double dSigma);
	CvGabor(int iMu, double iNu, double dSigma, double dF);
	CvGabor(double dPhi, double iNu);
	CvGabor(int iMu, double iNu);
	CvGabor(double dPhi, double iNu, double dSigma);
	CvGabor(double dPhi, double iNu, double dSigma, double dF);

	~CvGabor();
	bool IsInit();		//���ްٶ��ق�����������Ă��邩����
	long mask_width();	//̨������
	IplImage* get_image(int Type);
	bool IsKernelCreate();	//̨�����쐬����Ă��邩����
	long get_mask_width();	//̨�����ގ擾
	void Init(int iMu, double iNu, double dSigma, double dF);
	void Init(double dPhi, double iNu, double dSigma, double dF);//int�ł͓s���������̂�iNu��double�ɕύX
	void output_file(const char *filename, int Type);
	CvMat* get_matrix(int Type);
	void show(int Type);
	void normalize( const CvArr* src, CvArr* dst, double a, double b, int norm_type, const CvArr* mask );	//���K��
	void conv_img(IplImage *src, IplImage *dst, int Type);	//��ݍ���
	void conv_img_a(IplImage *src, IplImage *dst, int Type);//��ݍ���
	void conv_img_pt(IplImage *src, IplImage *dst, int Type, int*x, int*y, int len);
protected:
	double Sigma;	//�޳��֐��̕��U�l��
	double F;		//��Ԏ��g��
	double Kmax;	//�Ȗ͗l�̕��������ő�l
	double K;		//�Ȗ͗l�̕���
	double Phi;		//�ʑ��̾��
	bool bInitialised;	//�����������׸�
	bool bKernel;	//̨�������׸�
	long Width;		//̨������
	CvMat *Imag;	//�����̈�
	CvMat *Real;	//�����̈�

private:
	void creat_kernel();
};

#endif
