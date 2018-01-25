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
//定義値
//--------------------------------------------------------------------
#define PI 3.14159265
#define CV_GABOR_REAL 1
#define CV_GABOR_IMAG 2
#define CV_GABOR_MAG  3
#define CV_GABOR_PHASE 4

//--------------------------------------------------------------------
//ｶﾞﾎﾞｰﾙｸﾗｽ
//--------------------------------------------------------------------
class CvGabor{
public:
	//ｺﾝｽﾄﾗｸﾀ
	CvGabor(){};
	CvGabor(int iMu, double iNu, double dSigma);
	CvGabor(int iMu, double iNu, double dSigma, double dF);
	CvGabor(double dPhi, double iNu);
	CvGabor(int iMu, double iNu);
	CvGabor(double dPhi, double iNu, double dSigma);
	CvGabor(double dPhi, double iNu, double dSigma, double dF);

	~CvGabor();
	bool IsInit();		//ｶﾞﾎﾞｰﾙｶｰﾈﾙが初期化されているかﾁｪｯｸ
	long mask_width();	//ﾌｨﾙﾀｻｲｽﾞ
	IplImage* get_image(int Type);
	bool IsKernelCreate();	//ﾌｨﾙﾀが作成されているかﾁｪｯｸ
	long get_mask_width();	//ﾌｨﾙﾀｻｲｽﾞ取得
	void Init(int iMu, double iNu, double dSigma, double dF);
	void Init(double dPhi, double iNu, double dSigma, double dF);//intでは都合が悪いのでiNuをdoubleに変更
	void output_file(const char *filename, int Type);
	CvMat* get_matrix(int Type);
	void show(int Type);
	void normalize( const CvArr* src, CvArr* dst, double a, double b, int norm_type, const CvArr* mask );	//正規化
	void conv_img(IplImage *src, IplImage *dst, int Type);	//畳み込み
	void conv_img_a(IplImage *src, IplImage *dst, int Type);//畳み込み
	void conv_img_pt(IplImage *src, IplImage *dst, int Type, int*x, int*y, int len);
protected:
	double Sigma;	//ｶﾞｳｽ関数の分散値σ
	double F;		//空間周波数
	double Kmax;	//縞模様の方向成分最大値
	double K;		//縞模様の方向
	double Phi;		//位相ｵﾌｾｯﾄ
	bool bInitialised;	//初期化ﾁｪｯｸﾌﾗｸﾞ
	bool bKernel;	//ﾌｨﾙﾀﾁｪｯｸﾌﾗｸﾞ
	long Width;		//ﾌｨﾙﾀｻｲｽﾞ
	CvMat *Imag;	//虚数領域
	CvMat *Real;	//実数領域

private:
	void creat_kernel();
};

#endif
