#include "stdafx.h"
#include "egm.h"
#include "egmpfdr.h"

#define _USE_MATH_DEFINES
#include <math.h>

//ファイル出力先
#define PATH_OUTPUT "c:\\project\\result"

#define EGMD_CONST (1.0f/2)	// 歪み度にさらに乗算する値
#define EGMD_LINKLENGTH // st-dy間のリンク長の差
//#define EGMD_TYPEM		// リンク長の分散
#define EGMD_LINKDOT	// st-dy間のリンクの内積

namespace FDR {
void NormaliseImg(IplImage *in, IplImage **out, int w, int h);
void CutLowerValue(IplImage *in, float th);
}

// 微弱JETの除去 thが しきい値
namespace FDR {
	void CutLowerValue(IplImage *in, float th){
		if (in==NULL)return;
		if (in->depth != sizeof(float)*8)return;
		if (in->nChannels != 1)return;
		if (in->width * in->height < 1)return;

		float *data = (float*)in->imageData;
		int p, x, y, ws = in->widthStep / sizeof(float);
		for (y=0;y<in->height;y++) {
			for (x=0;x<in->width;x++) {
				p = y * ws + x;
				if (data[p] < th) data[p] = 0;
			}
		}
	}
}


// 顔検出クラス コンストラクタ
//-オプションの初期設定と動的変数ポインタの初期化
C_FACE_DETECT_RECOG::C_FACE_DETECT_RECOG()
{
	m_str_view_window_name[0] = '\0';
	m_sync_scale = FALSE;
	m_sweepfinding = TRUE;
	m_show_extended = TRUE;

	m_gabor = NULL;
	m_width_in=NULL;
	m_weight_dist=NULL;
	m_egm_times=NULL;
	m_egm_range=NULL;
	m_jet_in_disp = NULL;
	m_enmap = NULL;
	m_th_weak_feature = 5.0f;
	m_models=1;
	m_egmcnt=0;
	m_img_result = NULL;
	m_layers_mo = 0;
}

//デストラクタ 確保されっぱなしの動的変数を解放
C_FACE_DETECT_RECOG::~C_FACE_DETECT_RECOG()
{
	RELEASE_IPLIMAGE(m_jet_in_disp);
	RELEASE_ARRAY(m_gabor);
	if (m_layers_mo>0) {
		RELEASE_IPL_ARRAY(m_img_mo, m_layers_mo*m_models);
		RELEASE_IPL_ARRAY(m_jet_mo, m_layers_mo*m_gabor_ori*m_models);
		RELEASE_IPL_ARRAY(m_enmap, m_layers_mo);
	}
	RELEASE_IPLIMAGE(m_img_result);
}

// 前処理
int C_FACE_DETECT_RECOG::PreProcess(char **file_model)
{
	IplImage *img_tmp;

	// 記憶画像の生成
	m_img_mo = new IplImage*[m_layers_mo*m_models];
	m_jet_mo = new IplImage*[m_layers_mo*m_gabor_ori*m_models];
	m_enmap = new IplImage*[m_layers_mo];
	memset(m_enmap, 0, sizeof(IplImage*)*m_layers_mo);//使うとは限らないので0クリアしておく

	// 画面表示の準備
	if (m_show_filter) {
		RELEASE_IPLIMAGE(m_jet_in_disp);
		int w = m_width_in[0];
		m_jet_in_disp = cvCreateImage(cvSize(w, w*m_gabor_ori), IPL_DEPTH_8U, 3);
		cvSetZero(m_jet_in_disp);
		cvNamedWindow("jet_in");
		cvShowImage("jet_in", m_jet_in_disp);
	}

	// ガボールフィルタ生成
	m_gabor = new CvGabor[m_gabor_ori];
	double d;
	for (int r = 0; r < m_gabor_ori; r++) {
		d = M_PI * ((double)r / (double)m_gabor_ori);
		m_gabor[r].Init(d, m_Nu, m_Sigma, m_F);
	}

	// 記憶画像の読み込みとガボール変換
	for (int mi = 0; mi < m_models; mi++) {

		// 記憶画像の読み込み
		img_tmp = cvLoadImage( file_model[mi] );
		if (img_tmp == NULL) {
			return FALSE;
		}

		// 記憶画像の整形
		for (int m = 0; m < m_layers_mo; m++) {
			int idx = m + mi * m_layers_mo;
			FDR::NormaliseImg(img_tmp, &(m_img_mo[idx]), m_width_mo[m], m_width_mo[m]);

			//ヒストグラム均一化(記憶画像の背景部分がきちんと処理されている必要がある)
			cvEqualizeHist (m_img_mo[idx], m_img_mo[idx]);
		}
		cvReleaseImage(&img_tmp);

		// 記憶画像のガボール変換
		for (int m=0; m < m_layers_mo; m++) {
			for (int r=0; r < m_gabor_ori; r++) {
				int idx_img = m + mi * m_layers_mo;
				int idx_jet = r + idx_img * m_gabor_ori;
				m_jet_mo[idx_jet] = cvCreateImage(cvSize(m_img_mo[m]->width, m_img_mo[m]->height), IPL_DEPTH_32F, 1);
				m_gabor[r].conv_img(m_img_mo[idx_img], m_jet_mo[idx_jet], CV_GABOR_MAG);
				////微弱な応答の除去
				//FDR::CutLowerValue(m_jet_mo[m * m_gabor_ori + r], 0.1f);
			}
		}
	}

	// jetの画面表示
	if (m_show_filter) {
		int w = m_width_mo[0];
		IplImage *imgtmp, *imgtmp2, *imgdisp;
		imgdisp = cvCreateImage(cvSize(w*m_layers_mo*m_models, w*m_gabor_ori), IPL_DEPTH_8U, 3);
		cvSetZero(imgdisp);

		for (int mi = 0; mi < m_models; mi++) {
			for (int m=0; m < m_layers_mo; m++) {
				for (int r=0; r < m_gabor_ori; r++) {
					int idx_jet = r + (m + mi * m_layers_mo) * m_gabor_ori;
					imgtmp2 = cvCloneImage(m_jet_mo[idx_jet]);
					imgtmp = cvCloneImage(imgtmp2);
					cvNormalize(imgtmp, imgtmp2, 255.0, 0.0, CV_MINMAX);//255~0に正規化
					cvReleaseImage(&imgtmp);

					imgtmp = cvCreateImage(cvSize(imgtmp2->width, imgtmp2->height), IPL_DEPTH_8U, 3);
					cvt_32F1_to_8U3(imgtmp2, imgtmp);
					cvReleaseImage(&imgtmp2);

					cvSetImageROI(imgdisp, cvRect((m + mi * m_layers_mo)*w, r*w, imgtmp->width, imgtmp->height));
					cvCopyImage(imgtmp, imgdisp);
					cvResetImageROI(imgdisp);

					cvReleaseImage(&imgtmp);
				}
			}
		}

		cvNamedWindow("m_jet_mo", 1);
		cvShowImage ("m_jet_mo", imgdisp);
		
		cvReleaseImage(&imgdisp);

		// modelの画面表示(格子つき)
		C_GRID grid;
		grid.CreateRoundCornerSquareGrid(&m_grid_i, m_grid_res, m_grid_res, &m_grid_m);
		imgdisp = cvCreateImage(cvSize(w*m_models, w), IPL_DEPTH_8U, 3);
		for (int mi = 0; mi < m_models; mi++) {
			IplImage* imgtmp = cvCreateImage(cvSize(w, w), IPL_DEPTH_8U, 3);
			cvCvtColor(m_img_mo[mi*m_layers_mo], imgtmp, CV_GRAY2BGR);
			grid.DrawGrid(imgtmp, FALSE, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 0);

			cvSetImageROI(imgdisp, cvRect(mi*w, 0, w, w));
			cvCopyImage(imgtmp, imgdisp);
			cvResetImageROI(imgdisp);

			cvReleaseImage(&imgtmp);
		}
		cvNamedWindow("m_img_mo", 1);
		cvShowImage ("m_img_mo", imgdisp);
		cvReleaseImage(&imgdisp);
	}

	return TRUE;
}

// 極大値の重複区間除去
int C_FACE_DETECT_RECOG::PeakElimination(S_PEAK **p_listtop, float th_ov)
{
	int cnt=0;//消去された個数をカウント
	int dx, dy, dist;
	S_PEAK *p_top = *p_listtop;
	S_PEAK *p_prev = NULL;
	S_PEAK *p = p_top;
	S_PEAK *ppp;//一時退避用ダミー
	while( p ) {
		S_PEAK *pp_prev = NULL;
		S_PEAK *pp = p_top;
		int deleteflag = 0;
		while( pp ) {
			if (p != pp) {
				dx = abs(p->pos.x - pp->pos.x);
				dy = abs(p->pos.y - pp->pos.y);
				dist = (int)((p->scale/2 + pp->scale/2) * th_ov);
				if (dx < dist || dy < dist) {
					if (p->value > pp->value) {
						//ppがきえる
						if (pp_prev) {
							pp_prev->next = pp->next;
						}
						if (pp == p_top) {
							p_top = p_top->next;
						}
						ppp = pp;
						pp = pp->next;
						if (ppp->grid) delete ppp->grid;
						delete ppp;
						cnt++;
						continue;
					} else {
						//pがきえる
						deleteflag = 1;
						break;
					}
				}
			}
			pp_prev = pp;
			pp = pp->next;
		}

		if (deleteflag) {
			if (p_prev)
				p_prev->next = p->next;
			if (p == p_top)
				p_top = p_top->next;
			ppp = p;
			p = p->next;
			if (ppp->grid) delete ppp->grid;
			delete ppp;
			cnt++;
		} else {
			p_prev = p;
			p = p->next;
		}
	}
	*p_listtop = p_top;
	return cnt;
}

// 極大値探索
int C_FACE_DETECT_RECOG::PeakDetection(IplImage *img, int mw, float th_v, float th_ov, S_PEAK **p_listtop, S_PEAK **p_listlast)
{
	int w = img->width;
	int h = img->height;
	int ws = img->widthStep / (int)sizeof(float);
	float *data = (float*)img->imageData;
	S_PEAK *local_top = NULL;
	S_PEAK *peak = NULL;
	int cnt=0;

	for (int y = 1; y < h-1; y ++) {
		for (int x = 1; x < w-1; x ++) {
			int p = x + y * ws;
			if (data[p] > th_v)//しきい値処理
			//if (data[p-ws] > 0)//端っこを除去
			//if (data[p-1] > 0)
			//if (data[p+1] > 0)
			//if (data[p+ws] > 0)
			if (data[p] > data[p-ws-1])//8近傍と比較
			if (data[p] > data[p-ws])
			if (data[p] > data[p-ws+1])
			if (data[p] > data[p-1])
			if (data[p] > data[p+1])
			if (data[p] > data[p+ws-1])
			if (data[p] > data[p+ws])
			if (data[p] > data[p+ws+1]) {
				peak = new S_PEAK;
				peak->pos = cvPoint(x,y);
				peak->scale = mw;
				peak->value = data[p];
				peak->grid = NULL;
				peak->next = local_top;
				local_top = peak;
				cnt++;
				x++;//右隣はピークでないわけだから１つ飛ばす
			}
		}
	}
	cnt -= PeakElimination(&local_top, th_ov);
	*p_listtop = local_top;
	S_PEAK *p =  local_top;
	while(p) {
		if (! p->next) {
			*p_listlast = p;
			break;
		}
		p = p->next;
	}

	return cnt;
}

// リストの解放
void C_FACE_DETECT_RECOG::DeleteS_PEAK(S_PEAK *peak)
{
	while(peak) {
		S_PEAK *p = peak;
		peak = peak->next;
		if (p->grid) delete p->grid;
		delete p;
	}
}

// EGM計算
float C_FACE_DETECT_RECOG::FaceRecognition(IplImage *imgin, int wi, C_GRID **grid_r, int display)
{
	float v;
	IplImage **jet_in = new IplImage*[m_gabor_ori];

	//-入力画像のガボールフィルタ
	for (int r = 0; r < m_gabor_ori; r++) {
		jet_in[r] = cvCreateImage(cvGetSize(imgin), IPL_DEPTH_32F, 1);
		m_gabor[r].conv_img(imgin, jet_in[r], CV_GABOR_MAG);
		////--微弱な応答の除去
		//if (m_filter_noise)
		//	FDR::CutLowerValue(jet_in[r], 0.2f);
	}

	//画面表示の準備
	IplImage *img_disp = NULL;
	char str_tmp[1024];
	CvFont font;
	if (display>0) {
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
		CvSize sz;
		sz.width = imgin->width + m_width_mo[wi];
		sz.height = 48 + ((imgin->height > m_width_mo[wi])?imgin->height:m_width_mo[wi]);
		img_disp = cvCreateImage(sz, IPL_DEPTH_8U, 3);
		cvSetZero(img_disp);
	}

	int besti = 0;
	float bestv = 0;

	//{int mi = 0;
	for (int mi=0; mi < m_models; mi++) {
		int idx_img_mo = mi*m_layers_mo;
		int idx_jet_mo =m_gabor_ori * wi + m_gabor_ori * m_layers_mo * mi;

		//-egm classの準備
		C_GRID *grid = new C_GRID;
		grid->CreateRoundCornerSquareGrid(&m_grid_i, m_grid_res, m_grid_res, &m_grid_m);

		C_EGM_FDR* egm = new C_EGM_FDR;
		egm->SetSuperReso(FALSE);
		egm->SetParam(1, m_weight_dist[0]);
		egm->SetJet_st(m_jet_mo + idx_jet_mo, m_gabor_ori);
		egm->SetJet_dy(jet_in, m_gabor_ori);
		egm->SetGrid(grid);
		egm->SetNodeWeight(0);

		//画面表示の準備
		if (display>0) {
			//記憶顔に格子を張り付ける
			IplImage *img_tmp = cvCreateImage(cvGetSize(m_img_mo[idx_img_mo]), IPL_DEPTH_8U, 3);
			cvCvtColor(m_img_mo[idx_img_mo], img_tmp, CV_GRAY2BGR);
			grid->DrawGrid(img_tmp, FALSE, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 2);//格子描画
			cvSetImageROI(img_disp, cvRect(imgin->width, 0, img_tmp->width, img_tmp->height));
			cvCopy(img_tmp, img_disp);
			cvResetImageROI(img_disp);
			cvReleaseImage(&img_tmp);
		}

		for (int t = 0; t < m_egm_times[0]; t++) {
			int r = egm->EGM_FDR(1, m_egm_range[0]);

			if (display>=2 || grid_r)egm->GetGrid(grid);
			
			//途中経過の画面表示
			if (display>=2){

				//-入力画像に格子を貼る
				IplImage *img_tmp = cvCreateImage(cvSize(imgin->width, imgin->height), IPL_DEPTH_8U, 3);
				cvCvtColor(imgin, img_tmp, CV_GRAY2BGR);
				grid->DrawGrid(img_tmp, TRUE, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 1);//格子描画
				cvSetImageROI(img_disp, cvRect(0, 0, img_tmp->width, img_tmp->height));
				cvCopy(img_tmp, img_disp);
				cvResetImageROI(img_disp);
				cvReleaseImage(&img_tmp);

				//文字描画
				cvDrawRect(img_disp, cvPoint(0, img_disp->height-48), cvPoint(img_disp->width-1, img_disp->height-1), CV_RGB(0,0,0), CV_FILLED);

				//sprintf_s(str_tmp, 1024, "IN:%d, MO:%d", m_width_in[s], m_width_mo[m]);
				sprintf_s(str_tmp, 1024, "Count:%3d/%3d MvdNodes:%d", t+1, m_egm_times[0]+1, egm->m_lastmovednodes);
				cvPutText(img_disp, str_tmp, cvPoint(4, img_disp->height-24-4), &font, CV_RGB(255,255,255)); 

				sprintf_s(str_tmp, 1024, "VALUE:%.4lf Count:%3d/%3d MvdNodes:%4d", egm->GetValue());
				cvPutText(img_disp, str_tmp, cvPoint(4, img_disp->height-4), &font, CV_RGB(255,255,255)); 

				cvShowImage(m_str_view_window_name, img_disp);

				cvWaitKey(100);

				//printf("\rFace:%2d/%2d Layer:%2d/%2d Count:%3d/%3d MvdNodes:%4d", mi+1, m_models, s+1, 1+1, t+1, m_egm_times[s], egm->m_lastmovednodes);
			}

			if (r < 1) break;
		}
		v = egm->GetValue();
//		v = egm->GetDistortionValue();

		if (mi==0 || v>bestv) {
			bestv = v;
			besti = mi;
		}

		if (display==3) { //ファイル出力
			char fn[MAX_PATH];
			sprintf_s(fn, MAX_PATH, "%s\\EGM%02d.png", PATH_OUTPUT, m_egmcnt);
			cvSaveImage(fn, img_disp);
			m_egmcnt++;
		}
		if (display>=2)
			cvWaitKey(0);
		else if (display>=1)
			cvWaitKey(3000);

		delete egm;
		if (grid_r)
			*grid_r = grid;
		else
			delete grid;//grid_rに値があれば
	}

	for (int r = 0; r < m_gabor_ori; r ++)
		if (jet_in[r]) cvReleaseImage(jet_in + r);
	delete [] jet_in;

	if (img_disp) {
		cvReleaseImage(&img_disp);
	}
	//もしも不正解モデルが優勝したらエネルギ0を返す
	if (m_modeltype[besti] != 0) {
		bestv = 0;
	}

	return bestv;
}

// 顔検出
int C_FACE_DETECT_RECOG::FaceDetection(IplImage *imgin, S_PEAK **ret, int display, int store_enMap)
{
	int pyra_layers = m_layers_in;
	IplImage *img_in_0;		//正規化済みオリジナルスケール入力画像
	IplImage *img_in=NULL;
	IplImage **jet_in = new IplImage*[m_gabor_ori];
	memset(jet_in, 0, sizeof(IplImage*)*m_gabor_ori);

	CvFont font;
	char str_tmp[1024];
	if (display>0)
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);

	// 最大スケールの入力画像作成
	// これを縮小して使う
	int w0 = ((imgin->width > imgin->height)?imgin->height:imgin->width);
	FDR::NormaliseImg(imgin, &img_in_0, w0, w0);
//	FDR::NormaliseImg(imgin, &img_in_0, m_width_in[m_layers_in-1], m_width_in[m_layers_in-1]);

	//EGM_VALUETYPE *result_value = new EGM_VALUETYPE[pyra_layers];
	//S_RECT *result_rect = new S_RECT[pyra_layers];

	// 画面表示の準備
	IplImage *img_disp=NULL;
	if (display > 0) {
		CvSize sz;
		if (m_show_extended) {
			sz.width = m_width_in[0]*8 + m_width_mo[0]*8;
			sz.height = 48 + ((m_width_in[0] > m_width_mo[0])?m_width_in[0]*8:m_width_mo[0]*8);
		} else {
			sz.width = img_in_0->width + m_width_mo[0];
			sz.height = 48 + ((img_in_0->height > m_width_mo[0])?img_in_0->height:m_width_mo[0]);
		}
		img_disp = cvCreateImage(sz, IPL_DEPTH_8U, 3);
		cvNamedWindow("SweepFinding");
	}
	if (display>0) printf("\n");

	// 格子の初期設定
	//S_RECT r_stack = s_rect(0,0,1,1);//切り抜き領域

	int bestm = 0;

	//顔検出

	//-入力画像のダウンサンプリング
	img_in = cvCreateImage(cvSize(m_width_in[0], m_width_in[0]), IPL_DEPTH_8U, 1);
	cvResize(img_in_0, img_in);

	//-入力画像のガボールフィルタ
	for (int r = 0; r < m_gabor_ori; r++) {
		if (jet_in[r] != NULL) cvReleaseImage(jet_in + r);
	}
	for (int r = 0; r < m_gabor_ori; r++) {
		jet_in[r] = cvCreateImage(cvSize(img_in->width, img_in->height), IPL_DEPTH_32F, 1);
		m_gabor[r].conv_img(img_in, jet_in[r], CV_GABOR_MAG);
		//--微弱な応答の除去
		if (m_filter_noise)
			FDR::CutLowerValue(jet_in[r], m_th_weak_feature);
	}
	// jetの画面表示
	if (m_show_filter || display > 0) {
		int w = m_width_in[0];
		for (int r = 0; r < m_gabor_ori; r++) {
			IplImage *imgtmp;
			imgtmp = cvCreateImage(cvSize(jet_in[r]->width, jet_in[r]->height), IPL_DEPTH_8U, 3);
			cvt_32F1_to_8U3(jet_in[r], imgtmp);
			cvSetImageROI(m_jet_in_disp, cvRect(0*w, r*w, imgtmp->width, imgtmp->height));
			cvCopyImage(imgtmp, m_jet_in_disp);
			cvResetImageROI(m_jet_in_disp);
			cvReleaseImage(&imgtmp);
		}
		cvShowImage("jet_in", m_jet_in_disp);
		if (display==3) { //ファイル出力
			char fn[MAX_PATH];
			sprintf_s(fn, MAX_PATH, "%s\\jet_in.png", PATH_OUTPUT, m_jet_in_disp);
			cvSaveImage(fn, m_jet_in_disp);
			m_egmcnt++;
		}
	}

	//-平均記憶顔で全域探査マッチング
	//-記憶画像スケールでループ
	S_PEAK *peak = NULL;
	int peaks = 0;
	for (int m = 0; m < m_layers_mo; m ++) {
		int idx_jet_mo = m * m_gabor_ori;

		//-egm classの準備
		C_GRID *grid = new C_GRID;
		grid->CreateRoundCornerSquareGrid(&m_grid_i, m_grid_res, m_grid_res, &m_grid_m);

		C_EGM_FDR* egm = new C_EGM_FDR;
		egm->SetSuperReso(FALSE);
		egm->SetParam(1, 0);
		egm->SetJet_st(m_jet_mo + idx_jet_mo, m_gabor_ori);
		egm->SetJet_dy(jet_in, m_gabor_ori);
		egm->SetGrid(grid);
		egm->SetNodeWeight(m_node_weight);
//		egm->SetWeightDiff(m_weight_diff);
		
		//-全域探索
		IplImage *img_corr;
		egm->SweepFinding(0, &img_corr, ((m_sweepfinding>0)?m_sweepfinding:1));

		//-ピーク探索
		S_PEAK *peak_local=NULL, *peak_last=NULL;
		int peaks_local = PeakDetection(img_corr, m_width_mo[m], 0.77f, 0.5f, &peak_local, &peak_last);

		//途中経過の画面表示
		if (display>0){

			egm->GetGrid(grid);
			cvSetZero(img_disp);
			
			//-入力画像を貼り付け
			IplImage *img_tmp, *img_tmp2;
			if (m_show_extended) {
				img_tmp = cvCreateImage(cvSize(img_in->width, img_in->height), IPL_DEPTH_8U, 3);
				cvCvtColor(img_in, img_tmp, CV_GRAY2BGR);
				img_tmp2 = cvCreateImage(cvSize(img_in->width * 8, img_in->height * 8), IPL_DEPTH_8U, 3);
				cvResize(img_tmp, img_tmp2);
				cvReleaseImage(&img_tmp);
			} else {
				img_tmp2 = cvCreateImage(cvSize(img_in->width, img_in->height), IPL_DEPTH_8U, 3);
				cvCvtColor(img_in, img_tmp2, CV_GRAY2BGR);
			}
			
			//候補領域を描画
			//grid->DrawGrid(img_tmp2, TRUE, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 1);//格子描画
			S_PEAK *p = peak_local;
			while (p) {
				sprintf_s(str_tmp, 1024, "%.3f", p->value);
				if (m_show_extended) {
					cvDrawRect(img_tmp2, cvPoint((p->pos.x - p->scale/2)*8, (p->pos.y - p->scale/2)*8)
					,  cvPoint((p->pos.x + p->scale/2)*8, (p->pos.y + p->scale/2)*8), CV_RGB(255,255,255));
					cvPutText(img_tmp2, str_tmp, cvPoint((p->pos.x - p->scale/2)*8, (p->pos.y+p->scale/2)*8), &font, CV_RGB(255,255,255));
				} else {
					cvDrawRect(img_tmp2, p->pos - cvPoint(p->scale/2, p->scale/2),  p->pos + cvPoint(p->scale/2, p->scale/2), CV_RGB(255,255,255));
					cvPutText(img_tmp2, str_tmp, p->pos + cvPoint(-p->scale/2, p->scale/2), &font, CV_RGB(255,255,255));
				}
				p = p->next;
			}

			cvSetImageROI(img_disp, cvRect(0, 0, img_tmp2->width, img_tmp2->height));
			cvCopy(img_tmp2, img_disp);
			cvResetImageROI(img_disp);
			cvReleaseImage(&img_tmp2);

			//-記憶画像を貼り付け
			if (m_show_extended) {
				img_tmp = cvCreateImage(cvSize(m_img_mo[m]->width, m_img_mo[m]->height), IPL_DEPTH_8U, 3);
				cvCvtColor(m_img_mo[m], img_tmp, CV_GRAY2BGR);
				img_tmp2 = cvCreateImage(cvSize(m_img_mo[m]->width*8, m_img_mo[m]->height*8), IPL_DEPTH_8U, 3);
				cvResize(img_tmp, img_tmp2);
				cvReleaseImage(&img_tmp);
			} else {
				img_tmp2 = cvCreateImage(cvSize(m_img_mo[m]->width, m_img_mo[m]->height), IPL_DEPTH_8U, 3);
				cvCvtColor(m_img_mo[m], img_tmp2, CV_GRAY2BGR);
			}
			grid->DrawGrid(img_tmp2, FALSE, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 2);//格子描画
			if (m_show_extended)
				cvSetImageROI(img_disp, cvRect(img_in->width*8, 0, img_tmp2->width, img_tmp2->height));
			else
				cvSetImageROI(img_disp, cvRect(img_in_0->width, 0, img_tmp2->width, img_tmp2->height));
			cvCopy(img_tmp2, img_disp);
			cvResetImageROI(img_disp);
			cvReleaseImage(&img_tmp2);
			cvShowImage(m_str_view_window_name, img_disp);
		}//if display>0
		if (display > 0 || store_enMap) {
			//エネルギーマップの描画
			IplImage *img_en;
			if (m_show_extended)
				img_en = cvCreateImage(cvSize(img_corr->width*8, img_corr->height*8), IPL_DEPTH_8U, 3);
			else
				img_en = cvCreateImage(cvSize(img_corr->width, img_corr->height), IPL_DEPTH_8U, 3);

			float *buff_co = (float*)img_corr->imageData;
			BYTE *buff_en = (BYTE*)img_en->imageData;
			int p_co, yp_co;
			//int p_en, yp_en;
			int ws_co = img_corr->widthStep / sizeof(float);
			//int ws_en = img_en->widthStep;
			//CvScalar c;
			for (int y=0; y<img_corr->height; y++) {
				yp_co = y * ws_co;
				//yp_en = y * ws_en;
				for (int x=0; x<img_corr->width; x++) {
					p_co = x + yp_co;
					//p_en = x * 3 + yp_en;
					int iv = (int)(buff_co[p_co] * 255.0f);
					//c = N_EGM::GetDisplayColor(iv);
					//buff_en[p_en  ] = c.val
					//buff_en[p_en+1] = (BYTE)((c>>8) & 0xFF);
					//buff_en[p_en+2] = (BYTE)((c>>16) & 0xFF);
					if (m_show_extended)
						cvRectangle(img_en, cvPoint(x*8,y*8), cvPoint(x*8+8,y*8+8), N_EGM::GetDisplayColor(iv), -1);
					else
						cvRectangle(img_en, cvPoint(x,y), cvPoint(x+8,y+8), N_EGM::GetDisplayColor(iv), -1);
				}
			}
			if (m_show_extended) {
				S_PEAK *p = peak_local;
				while (p) {
						cvDrawRect(img_en, cvPoint(p->pos.x*8, p->pos.y*8)
						,  cvPoint(p->pos.x*8+8, p->pos.y*8+8), CV_RGB(0,0,0));
					p = p->next;
				}
			}
			//-記憶しておく
			if (m_enmap[m] != NULL)
				cvReleaseImage(m_enmap + m);
			m_enmap[m] = cvCloneImage(img_en);

			if (display>0) {
				//表示終わり，wait
				cvWaitKey(100);
				cvShowImage("SweepFinding", img_en);
				if (display==3) { //ファイル出力
					char fn[MAX_PATH];
					sprintf_s(fn, MAX_PATH, "%s\\SimMapL%d.png", PATH_OUTPUT, m+1);
					cvSaveImage(fn, img_en);
					sprintf_s(fn, MAX_PATH, "%s\\ResultL%d.png", PATH_OUTPUT, m+1);
					cvSaveImage(fn, img_disp);
				}
			}

			cvReleaseImage(&img_en);
		}//if (display > 0 || store_enMap)

		if (display>1) cvWaitKey(0);

		//各ピークでEGMを行って評価値を更新

		////EGMで評価値を更新
		//S_PEAK *p = peak_local;
		//while (p) {
		//	IplImage *img = cvCreateImage(cvSize(m_width_in[0], m_width_in[0]), IPL_DEPTH_8U, 1);
		//	int x = (p->pos.x - p->scale/2) * img_in_0->width / m_width_in[0];
		//	int y = (p->pos.y - p->scale/2) * img_in_0->height / m_width_in[0];
		//	int w = p->scale * img_in_0->height / m_width_in[0];
		//	cvSetImageROI(img_in_0, cvRect(x, y, w, w));
		//	cvResize(img_in_0, img);
		//	cvResetImageROI(img_in);
		//	float v = FaceRecognition(img, 0, display);
		//	if (display>=2) printf("(%3d,%3d,%3d) %.3f to %.3f\n", p->pos.x, p->pos.y, p->scale, p->value, v);
		//	p->value = v;
		//	cvReleaseImage(&img);
		//	p = p->next;
		//}

		//この層で得られたピークのリストを全体のリストに結合
		if (peak_last)
			peak_last->next = peak;
		peak = peak_local;
		peaks += peaks_local;

		delete egm;
		delete grid;
		cvReleaseImage(&img_corr);
	}
	
	//-スケール間で同位置の決戦
	peaks -= PeakElimination(&peak, 0.1f);

	//EGMで評価値を更新
	S_PEAK *p = peak;
	while (p) {
		IplImage *img = cvCreateImage(cvSize(m_width_in[0], m_width_in[0]), IPL_DEPTH_8U, 1);
		int x = (p->pos.x - p->scale/2) * img_in_0->width / m_width_in[0];
		int y = (p->pos.y - p->scale/2) * img_in_0->height / m_width_in[0];
		int w = p->scale * img_in_0->height / m_width_in[0];
		cvSetImageROI(img_in_0, cvRect(x, y, w, w));
		cvResize(img_in_0, img);
		cvResetImageROI(img_in);
		float v = FaceRecognition(img, 0, &(p->grid), display);
		if (display>0) printf("(%3d,%3d,%3d) %.3f to %.3f\n", p->pos.x, p->pos.y, p->scale, p->value, v);
		p->value = v;
		cvReleaseImage(&img);
		p = p->next;
	}

	//最終決戦
	peaks -= PeakElimination(&peak, 0.75f);
	
	//paekの座標系を元のスケールに変換
	//ついでに最大値探索をしてm_r_result, m_v_resultに代入
	S_PEAK *pp = peak;
	float vmax=0;
	CvRect rmax = cvRect(0,0,1,1);
	while (pp) {
		if (imgin->width > imgin->height) {//横長
			int xmod = (imgin->width - imgin->height) / 2;//片側の切り抜き幅[px]
			pp->pos.x = pp->pos.x * imgin->height / m_width_in[0] + xmod;
			pp->pos.y = pp->pos.y * imgin->height / m_width_in[0];
			pp->scale = pp->scale * imgin->height / m_width_in[0];
		} else {
			int ymod = (imgin->height - imgin->width) / 2;
			pp->pos.x = pp->pos.x * imgin->width / m_width_in[0];
			pp->pos.y = pp->pos.y * imgin->width / m_width_in[0] + ymod;
			pp->scale = pp->scale * imgin->width / m_width_in[0];			
		}
		if (pp->value > vmax || peak == pp) {
			vmax = pp->value;
			int s = pp->scale/2;
			rmax = cvRect(pp->pos.x-s, pp->pos.y-s, pp->scale, pp->scale);
		}
		pp = pp->next;
	}

	//結果画像を描く(display>0の場合のみ)
	if (display > 0) {
		if (m_img_result) cvReleaseImage(&m_img_result);
		m_img_result = cvCreateImage(cvGetSize(imgin), IPL_DEPTH_8U, 3);
		if (imgin->nChannels == 3)
			cvCopyImage(imgin, m_img_result);
		else
			cvCvtColor(imgin, m_img_result, CV_GRAY2BGR);
		S_PEAK *pp = peak;
		char str[64];
		while (pp) {
			int s = pp->scale/2;
			cvRectangle(m_img_result, cvPoint(pp->pos.x-s, pp->pos.y-s), cvPoint(pp->pos.x+s, pp->pos.y+s), CV_RGB(255, 255, 0));
			sprintf_s(str, 64, "%.03f", pp->value);
			cvPutText(m_img_result, str, cvPoint(pp->pos.x-s, pp->pos.y+s), &font, CV_RGB(255, 255, 0));
			if (pp->grid) {
				cvSetImageROI(m_img_result, cvRect(pp->pos.x-s, pp->pos.y-s, pp->scale, pp->scale));
				IplImage *imgTmp = cvCreateImage(cvGetSize(m_img_result), IPL_DEPTH_8U, 3);
				cvCopyImage(m_img_result, imgTmp);
				pp->grid->DrawGrid(imgTmp, 1, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 1);//格子描画
				cvCopyImage(imgTmp, m_img_result);
				cvResetImageROI(m_img_result);
				cvReleaseImage(&imgTmp);
			}
			pp = pp->next;
		}		
		if (display==3) { //ファイル出力
			char fn[MAX_PATH];
			sprintf_s(fn, MAX_PATH, "%s\\result.png", PATH_OUTPUT);
			cvSaveImage(fn, m_img_result);
		}
	}

	m_r_result = rmax;
	m_v_result = vmax;

	// 変数解放
	cvReleaseImage(&img_in_0);
	if (img_in) cvReleaseImage(&img_in);
	for (int r = 0; r < m_gabor_ori; r ++) {
		if (jet_in[r]) cvReleaseImage(jet_in + r);
	}
	delete [] jet_in;
	
	//delete [] result_value;
	//delete [] result_rect;

	if (ret)
		*ret = peak;
	else
		DeleteS_PEAK(peak);

	return peaks;
}

//エネルギーマップを1枚の画像にまとめて返す
IplImage *C_FACE_DETECT_RECOG::GetEnergyMap() {
	if (m_enmap == NULL) return NULL;
	for (int i = 0; i < m_layers_mo; i++)
		if (m_enmap[i] == NULL) return NULL;

	CvSize sz = cvGetSize(m_enmap[0]);
	IplImage *imgOut = cvCreateImage(sz, IPL_DEPTH_8U, 3);
	cvSetZero(imgOut);
	int pitch_h = sz.height / m_layers_mo;
	for (int m = 0; m < m_layers_mo; m++) {
		cvSetImageROI(imgOut, cvRect(0, m*pitch_h, sz.width, pitch_h));
		cvResize(m_enmap[m], imgOut, 1);
		cvResetImageROI(imgOut);
	}

	return imgOut;
}

IplImage *C_FACE_DETECT_RECOG::GetResultImage() {
	if (m_img_result == NULL) return NULL;
	return cvCloneImage(m_img_result);
}

C_EGM_FDR::C_EGM_FDR() {
	m_grid = NULL;
	m_jet_st = NULL;
	m_jet_dy = NULL;
	m_grid_backup = new C_GRID;
	m_grid = new C_GRID;
	m_th_lock = 0;
	m_v_log = new int[4];
	m_v_log[0] = 1;
	m_v_log[1] = 3;
	m_v_log[2] = 5;
	m_v_log[3] = 7;
	m_v_log_cnt = 0;
//	m_weight_diff = 1.0f;
//	C_EGM();
}

C_EGM_FDR::~C_EGM_FDR() {
	delete [] m_v_log;
	m_v_log = NULL;
	C_EGM::~C_EGM();
}

// 指定回数EGMを実行，実行された回数を返す
int C_EGM_FDR::EGM_FDR(int times, int range, int reso)
{
	int cnt = 0;
	while (cnt < times)
	{
		if (reso > 1) {
			if (EGM_FDR_SR_Main(range, reso) <= 0)
			break; //エラーまたは全てのノードが不動のときに成立
		} else {
			if (EGM_FDR_Main(range) <= 0)
			break; //エラーまたは全てのノードが不動のときに成立
		}
		
		//振動検知
		m_v_log[m_v_log_cnt] = (int)(m_value * 10000);
		m_v_log_cnt ++;
		if (m_v_log_cnt>3)m_v_log_cnt=0;
		if (m_v_log[0] == m_v_log[2] && m_v_log[1] == m_v_log[3]) {
			m_lastmovednodes = -1;
			break;
		}

		cnt ++;
	}
	return cnt;
}

//超解像度計算，指定された小数点つき座標のJet求める，固定側
float C_EGM_FDR::GetJet_st(S_POINT pos, int dim)
{
	CvPoint ipos = S_POINTtoCvPoint(pos, m_width_st, m_height_st);
	int idx = (ipos.y * m_width_st + ipos.x) * m_jetdim + dim;

	float &a = m_jet_st[idx];
	float &b = m_jet_st[idx+m_jetdim];//右
	float &c = m_jet_st[idx+m_jetdim * m_width_st];//下
	float &d = m_jet_st[idx+m_jetdim * m_width_st+m_jetdim];//右下
	
	float x = pos.x * m_width_st - (float)ipos.x;
	float y = pos.y * m_height_st - (float)ipos.y;
	return  ( a * (1 - x) * (1 - y)
			+ b *      x  * (1 - y)
			+ c * (1 - x) *      y
			+ d *      x  *      y
			);
}

//超解像度計算，指定された小数点つき座標のJet求める，移動側
float C_EGM_FDR::GetJet_dy(S_POINT pos, int dim)
{
	CvPoint ipos = S_POINTtoCvPoint(pos, m_width_dy, m_height_dy);
	int idx = (ipos.y * m_width_dy + ipos.x) * m_jetdim + dim;

	float &a = m_jet_dy[idx];
	float &b = m_jet_dy[idx+m_jetdim];//右
	float &c = m_jet_dy[idx+m_jetdim * m_width_dy];//下
	float &d = m_jet_dy[idx+m_jetdim * m_width_dy+m_jetdim];//右下
	
	float x = pos.x * m_width_dy - (float)ipos.x;
	float y = pos.y * m_height_dy- (float)ipos.y;
	return  ( a * (1 - x) * (1 - y)
			+ b *      x  * (1 - y)
			+ c * (1 - x) *      y
			+ d *      x  *      y
			);
}

// 全域探査，超解像度計算設定は無視される
// 最大値をとった座標とエネルギがクラス内に記憶される
// img_retを指定すればエネルギマップを返す
// pitchはfor文に直接 x += pitch といった形で入る
int C_EGM_FDR::SweepFinding(int display, IplImage **img_ret, int pitch)
{
	float maxsim=-2;
	int maxx=0, maxy=0;
	IplImage *img_disp, *img_return;
	float *buff_ret;
	if (img_ret) {
		img_return = cvCreateImage(cvSize(m_width_dy, m_height_dy), IPL_DEPTH_32F, 1);
		buff_ret = (float*)img_return->imageData;
	}

	// -格子情報
	SDLNODE *pnode = m_grid->GetBuffer();//現在の格子

	m_grid->Render(m_width_st, m_height_st, m_width_dy, m_height_dy);

	int nodes = m_grid->GetNodeBufferLength();//ノードバッファ長
	int anc = m_grid->GetActiveNodeCount();//アクティブなノードの数

	// 画面表示用
	if (display) {
		img_disp = cvCreateImage(cvSize(m_width_dy, m_height_dy), IPL_DEPTH_8U, 3);
		cvSetZero(img_disp);
	}

	float ave_weight = 1.0f / anc;

	for(int y=0; y <= m_height_dy - m_height_st; y+=pitch ) {
		for(int x=0; x <= m_width_dy - m_width_st; x+=pitch ) {

			float v=0;
			float sim=0;
			float norm_dy=0, norm_st=0;

			for(int i=0; i < nodes; i++) {

				if ( ! pnode[i].active ) continue;

				pnode[i].pos_dyi.x = x + pnode[i].pos_sti.x;
				pnode[i].pos_dyi.y = y + pnode[i].pos_sti.y;
				int idx_st = ( pnode[i].pos_sti.y * m_width_st + pnode[i].pos_sti.x) * m_jetdim;
				int idx_dy = ( pnode[i].pos_dyi.y * m_width_dy + pnode[i].pos_dyi.x) * m_jetdim;
				float sim=0;
				
				//内積計算で一致度を求める
				float norm_dy=0, norm_st=0;
				for(int d = 0; d < m_jetdim; d ++) {
					float dy = m_jet_dy[idx_dy + d];
					float st = m_jet_st[idx_st + d];
					sim += st * dy;
					norm_dy += dy * dy;
					norm_st += st * st;
				}
				norm_dy = EGM_SQRT(norm_dy);
				norm_st = EGM_SQRT(norm_st);

				// 分母が0になるようだったら一致度は0にする
				if( norm_dy == 0 || norm_st == 0)
					sim = 0;
				else
					sim = sim / (norm_st * norm_dy);

				// 重みをかけて全体の一致度に加算
				v += sim * pnode[i].weight;
			}

			// 最大値だったら位置とエネルギを記憶
			if (v > maxsim) {
				maxsim = v;
				maxx = x;
				maxy = y;
			}
			
			// エネルギを配列に書き込む
			if (img_ret) {
				buff_ret[x+m_width_st/2 + (y+m_height_st/2) * (img_return->widthStep>>2)] = v;
			}

			// エネルギマップに打点
			if (display) {
				int iv = (int)(v * 255.0f);
				cvRectangle(img_disp, cvPoint(x+m_width_st/2,y+m_height_st/2), cvPoint(x+m_width_st/2,y+m_height_st/2), N_EGM::GetDisplayColor(iv));
			}
		}
	}

	// クラス内部状態の更新
	//-格子を最大地点に移動
	for(int i=0; i < nodes; i++) {
		if ( ! pnode[i].active ) continue;
		pnode[i].pos_dyi.x = maxx + pnode[i].pos_sti.x;
		pnode[i].pos_dyi.y = maxy + pnode[i].pos_sti.y;
	}
	m_grid->UnRender(m_width_st, m_height_st, m_width_dy, m_height_dy);
	m_value = maxsim;
	m_lastmovednodes = m_grid->GetActiveNodeCount();//全ノードが移動したことにしておく

	if(display) {
		//最大地点をとった座標の，各ノードの一致度を再計算
		int x = maxx;
		int y = maxy;
		for(int i=0; i < nodes; i++) {
			if ( ! pnode[i].active ) continue;
			pnode[i].pos_dyi.x = x + pnode[i].pos_sti.x;
			pnode[i].pos_dyi.y = y + pnode[i].pos_sti.y;
			int idx_st = ( pnode[i].pos_sti.y * m_width_st + pnode[i].pos_sti.x) * m_jetdim;
			int idx_dy = ( pnode[i].pos_dyi.y * m_width_dy + pnode[i].pos_dyi.x) * m_jetdim;
			float sim=0;
			float norm_dy=0, norm_st=0;
			for(int d = 0; d < m_jetdim; d ++) {
				sim += m_jet_st[idx_st + d] * m_jet_dy[idx_dy + d];
				norm_dy += m_jet_dy[idx_dy + d] * m_jet_dy[idx_dy + d];
				norm_st += m_jet_st[idx_st + d] * m_jet_st[idx_st + d];
			}
			norm_dy = EGM_SQRT(norm_dy);
			norm_st = EGM_SQRT(norm_st);
			// 分母が0になるようだったら0にする
			if( norm_dy == 0 || norm_st == 0)
				sim = 0;
			else
				sim = sim / (norm_st * norm_dy);
			pnode[i].value = sim;
		}

		// エネルギマップ画面表示, 8倍に拡大
		IplImage *img_tmp = cvCreateImage(cvSize(img_disp->width*8, img_disp->height*8), IPL_DEPTH_8U, 3);
		cvResize(img_disp, img_tmp, 0);
		CvPoint pos = cvPoint((maxx+m_width_st/2)*8-4, (maxy+m_height_st/2)*8-4);
		cvDrawRect(img_tmp, pos, pos + cvPoint(8,8), CV_RGB(0,0,0),1);
		cvShowImage("SweepFinding", img_tmp);
		cvReleaseImage(&img_tmp);

		cvReleaseImage(&img_disp);//ここでエネルギマップを解放
	}

	if (img_ret) {
		*img_ret = img_return;//エネルギーマップを画像としてリターン
	}

	return m_lastmovednodes;
}

int C_EGM_FDR::EGM_FDR_Main(int range)
{
	// 変数宣言と初期化
	// -更新用の格子を記憶する領域
	C_GRID grid_next;
	grid_next.Copy(m_grid);

	// -格子情報
	SDLNODE *node_now = m_grid->GetBuffer();//現在の格子
	SDLNODE *node_next = grid_next.GetBuffer();//更新後の格子

	// -ループカウンタ
	int iNode, iDelta, iLink, i;

	// -探索範囲を格納する配列

	////8近傍最急降下法(森江研)
	//CvPoint deltaxy[] = {
	//	cvPoint( 0, 0),
	//	cvPoint(-1,-1), cvPoint( 0,-1), cvPoint( 1,-1),
	//	cvPoint(-1, 0)                , cvPoint( 1, 0),
	//	cvPoint(-1, 1), cvPoint( 0, 1), cvPoint( 1, 1)
	//};
	//int delta_len = (int)(sizeof(deltaxy) / sizeof(CvPoint));
	// ↓に統合した．range=1で同じ動作になる

	// 矩形領域内で探索
	// 動的配列を利用することに注意
	CvPoint *deltaxy;
	int delta_len;
	{
		int r = range;//探索範囲
		int x, y, p = 1;
		delta_len = (2 * r + 1) * (2 * r + 1);
		deltaxy = new CvPoint[delta_len];
		deltaxy[0] = cvPoint(0, 0);//中心は配列の先頭に入れておく

		for ( y = -r; y <= r; y++ ) {
			for ( x = -r; x <= r; x++) {
				if (x==0 && y==0) continue;
				deltaxy[p] = cvPoint(x, y);
				p++;
			}
		}
	}

	// -ループ制御用
	int nodes = m_grid->GetNodeBufferLength();//ノードバッファ長
	int movednodes = 0;// 移動したノードの数

	// 座標参照用
	CvPoint pos_st, pos_dy;
	//S_POINT s_pos_dy;

	// データ参照用
	int idx_st, idx_dy;

	// エネルギ計算用
	EGM_VALUETYPE norm_dy, norm_st;
	EGM_VALUETYPE en_jetsim, en_jetdist, en_node, en_all = 0;
	EGM_VALUETYPE en_node_winner;
	const EGM_VALUETYPE const_unti_vibration = 0;
	int idx_winner;

	// 全てのノードについてループ
	for (iNode = 0; iNode < nodes; iNode ++) {

		if (node_now[iNode].active == FALSE) continue;

		// 現在注目しているノードのstatic側座標を取得
		//pos_st = S_POINTtoCvPoint(node_now[iNode].pos_st, m_width_st, m_height_st);
		pos_st = node_now[iNode].pos_sti;

		if(pos_st.x < 0) continue;
		if(pos_st.x >= m_width_st)  continue;
		if(pos_st.y < 0) continue;
		if(pos_st.y >= m_height_st) continue;

		idx_st = (pos_st.y * m_width_st + pos_st.x) * m_jetdim;

		// 最大値探索用変数の初期化
		en_node_winner = node_now[iNode].value;//前回の計算の結果を初期値にする
		//en_node_winner = 0;
		idx_winner = 0;

		// dynamic側の注目座標を変えながらエネルギ最大の点を探す
		for(iDelta = 0; iDelta < delta_len; iDelta++) {

			// 現在注目しているノードのdynamic側座標を取得
			//pos_dy = deltaxy[iDelta] + S_POINTtoCvPoint(node_now[iNode].pos_dy, m_width_dy, m_height_dy);
			//s_pos_dy = CvPointtoS_POINT(pos_dy, m_width_dy, m_height_dy);
			pos_dy = deltaxy[iDelta] + node_now[iNode].pos_dyi;

			if(pos_dy.x < 0) continue;
			if(pos_dy.x >= m_width_dy)  continue;
			if(pos_dy.y < 0) continue;
			if(pos_dy.y >= m_height_dy) continue;
				
			idx_dy = (pos_dy.y * m_width_dy + pos_dy.x) * m_jetdim;

			// Jetの内積を求める -> en_jetsim
			norm_st = 0;
			norm_dy = 0;
			en_jetsim = 0;

			if ( m_jetdim > 1 ) {

				for(i = 0; i < m_jetdim; i ++) {
					en_jetsim += m_jet_st[idx_st + i] * m_jet_dy[idx_dy + i];
					norm_dy += m_jet_dy[idx_dy + i] * m_jet_dy[idx_dy + i];
					norm_st += m_jet_st[idx_st + i] * m_jet_st[idx_st + i];
				}
				norm_dy = EGM_SQRT(norm_dy);
				norm_st = EGM_SQRT(norm_st);

				// 分母が0になるようだったら内積の最低値(-1)にする
				if( norm_dy == 0 || norm_st == 0)
					en_jetsim = (EGM_VALUETYPE)0;
				else
					en_jetsim = en_jetsim / (norm_st * norm_dy);

				//減算項
				//ノルムの差をとって大きいほうのノルムで割る
				//float diff = fabs(norm_dy - norm_st) / ((norm_dy>norm_st)?norm_dy:norm_st);
				//en_jetsim -= diff * m_weight_diff;

			} else {
				en_jetsim = 1.0f - fabs(m_jet_st[idx_st] - m_jet_dy[idx_dy]) / 255.0f;
			}
			// 格子の歪みを計算 -> en_jetdist
			en_jetdist = 0;

			// st-dy間のリンク長の差　|固定側リンク - 移動側リンク|/固定側リンク
#ifdef EGMD_LINKLENGTH
			{
				//リンクの数を数える
				int Lc=0;
				EGM_VALUETYPE v=0;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//リンク先のインデックス
					CvPoint vLink_dy = cvPoint(
						node_now[n].pos_dyi.x - pos_dy.x,
						node_now[n].pos_dyi.y - pos_dy.y
					);
					CvPoint vLink_st = cvPoint(
						node_now[n].pos_sti.x - node_now[iNode].pos_sti.x,
						node_now[n].pos_sti.y - node_now[iNode].pos_sti.y
					);

					EGM_VALUETYPE f = cvp_size(vLink_st - vLink_dy) / cvp_size(vLink_st);
					v += f * f;

					Lc++;//リンクの数を数えておく
				}

				// 歪み度 = 
				en_jetdist += v / Lc;
			}
#endif
#ifdef EGMD_TYPEM
			// M式改：リンク長の分散=ave(|リンク長の平均-リンク長|/リンク長の平均)
			{
				//リンクの数を数える
				int Lc=0;
				EGM_VALUETYPE ave=0, v=0;
				int n;

				// リンク長の平均を求める
				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;
					n = node_now[iNode].link[iLink];//リンク先のインデックス

					CvPoint vLink_dy = node_now[n].pos_dyi - pos_dy;
					ave += cvp_size(vLink_dy);
					Lc++;//リンクの数を数えておく
				}
				if (Lc == 0) break;
				ave /= Lc;
				if (ave == 0) break;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					CvPoint vLink_dy = node_now[n].pos_dyi - pos_dy;
					EGM_VALUETYPE f = ave - cvp_size(vLink_dy);
					v += EGM_SQRT(f * f);
					//EGM_VALUETYPE f = fabs(ave - s_size(vLink_dy)/ave);
					//v += f;// * f;
				}

				// 歪み度 = 
				en_jetdist += v / Lc;
			}
#endif
#ifdef EGMD_LINKDOT
			// 案２'：リンクのベクトルをst, dy側それぞれで作り
			// 1-MIN(内積)を歪み度とする
			{
				//リンクの数を数える
				int Lc=0;
				EGM_VALUETYPE v=1.0f;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//リンク先のインデックス
					CvPoint vLink_dy = node_now[n].pos_dyi - pos_dy;
					CvPoint vLink_st = node_now[n].pos_sti - node_now[iNode].pos_sti;

					// 内積をとる
					EGM_VALUETYPE f = vLink_dy * vLink_st;

					if (f<v) v=f;

					Lc++;//リンクの数を数えておく
				}

				// 歪み度 = 1 - 内積の平均
				//en_jetdist += 1.0f - v;
				en_jetdist += 1.0f - v / Lc;

			}
#endif
			// 案１：格子の移動距離　|固定側座標 - 移動側座標|
			//{
			//	en_jetdist += s_size(node_now[iNode].pos_st - s_pos_dy);
			//}
	
			// 重みをつけてノードのエネルギを計算 -> en_node
			en_node = m_weight_sim * en_jetsim - m_weight_dist * en_jetdist * EGMD_CONST;

			// 最大値をとったエネルギを記憶
//			if(en_node > en_node_winner + const_unti_vibration){
			if(en_node > en_node_winner + const_unti_vibration || iDelta == 0 ){
				en_node_winner = en_node;
				idx_winner = iDelta;
			}

			// ノードに固定属性がついているときは
			// ループせずに抜ける(ループの１回目が移動しない場合のエネルギ計算)
			if (node_now[iNode].lock == TRUE) break;
			//if (norm_st < m_th_lock)
			//	node_next[iNode].lock = TRUE;

			//if (node_next[iNode].lock)
			//	break;

		} // next iDelta

		// 他のノードと衝突したときの処理をするときはここに書く

		// エネルギが最大になる点が中心でなかったら 格子を移動する
		if ( deltaxy[idx_winner].x != 0 || deltaxy[idx_winner].y != 0 ) {
			//node_now[iNode].pos_dyi = deltaxy[idx_winner] + node_now[iNode].pos_dyi;
			node_next[iNode].pos_dyi = deltaxy[idx_winner] + node_now[iNode].pos_dyi;
				//CvPointtoS_POINT(
				//						deltaxy[idx_winner]
				//						+ S_POINTtoCvPoint(node_now[iNode].pos_dy, m_width_dy, m_height_dy)
				//						, m_width_dy
				//						, m_height_dy);
			movednodes ++;
		}

		// エネルギを記憶
		node_next[iNode].value = en_node_winner;

		// このノードのエネルギを全体のエネルギに加算
		en_all += en_node_winner * node_now[iNode].weight;

	} // next iNode

	// ノードの移動をメンバ変数に反映
	m_grid->Copy(&grid_next);

	//// 歪み度を計算し，エネルギから差し引く
	//for (iNode = 0; iNode < nodes; iNode ++) {

	//	if (node_now[iNode].active == FALSE) continue;
	//	if (node_now[iNode].lock == TRUE) continue;//端っこのノードは減算しない

	//	// 現在注目しているノードのstatic側座標を取得
	//	pos_dy = node_now[iNode].pos_dy;

	//	EGM_VALUETYPE xsum=0, ysum=0;
	//	int n;
	//	for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
	//		n = node_now[iNode].link[iLink];
	//		if (n < 0) continue;
	//		xsum += node_now[n].pos_dy.x - pos_dy.x;
	//		ysum += node_now[n].pos_dy.y - pos_dy.y;
	//	}
	//	en_all -= m_weight_dist * EGM_SQRT(xsum * xsum + ysum * ysum);
	//}

	// エネルギを最大1.0になるように規格化
	// ただし，0で割らないようにする
	m_value = en_all;

	//int c = m_grid->GetActiveNodeCount();
	//if (c > 0)
	//	m_value /= c;

	//if (m_weight_sim != 0)
	//	m_value /= m_weight_sim;

	m_lastmovednodes = movednodes;

	// 動的配列の解放
	delete [] deltaxy;

	// 移動したノードの数を返す
	return movednodes;
}


//歪み度だけ計算して返す
EGM_VALUETYPE C_EGM_FDR::GetDistortionValue()
{
	SDLNODE *node_now = m_grid->GetBuffer();//現在の格子
	int nodes = m_grid->GetNodeBufferLength();

	EGM_VALUETYPE en_jetdist = 0, en_all = 0;
	int Nc=0;

	// 全てのノードについてループ
	for (int iNode = 0; iNode < nodes; iNode ++) {

		if (node_now[iNode].active == FALSE) continue;

		// st-dy間のリンク長の差　|固定側リンク - 移動側リンク|/固定側リンク
		{
			//リンクの数を数える
			int Lc=0;
			EGM_VALUETYPE v=0;
			int n;

			for (int iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
				if (node_now[iNode].link[iLink] < 0) continue;

				n = node_now[iNode].link[iLink];//リンク先のインデックス
				CvPoint vLink_dy = cvPoint(
					node_now[n].pos_dyi.x - node_now[iNode].pos_dyi.x,
					node_now[n].pos_dyi.y - node_now[iNode].pos_dyi.y
				);
				CvPoint vLink_st = cvPoint(
					node_now[n].pos_sti.x - node_now[iNode].pos_sti.x,
					node_now[n].pos_sti.y - node_now[iNode].pos_sti.y
				);

				EGM_VALUETYPE f = cvp_size(vLink_st - vLink_dy) / cvp_size(vLink_st);
				v += f * f;

				Lc++;//リンクの数を数えておく
			}

			// 歪み度 = 
			en_jetdist += v / Lc;
		}

		// 案２'：リンクのベクトルをst, dy側それぞれで作り
		// 1-MIN(内積)を歪み度とする
		{
			//リンクの数を数える
			int Lc=0;
			EGM_VALUETYPE v=1.0f;
			int n;

			for (int iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
				if (node_now[iNode].link[iLink] < 0) continue;

				n = node_now[iNode].link[iLink];//リンク先のインデックス
				CvPoint vLink_dy = node_now[n].pos_dyi - node_now[iNode].pos_dyi;
				CvPoint vLink_st = node_now[n].pos_sti - node_now[iNode].pos_sti;

				// 内積をとる
				EGM_VALUETYPE f = vLink_dy * vLink_st;

				if (f<v) v=f;

				Lc++;//リンクの数を数えておく
			}

			// 歪み度 = 1 - 内積の平均
			//en_jetdist += 1.0f - v;
			en_jetdist += 1.0f - v / Lc;

		}
		
		en_all += en_jetdist / 2.0f;

		Nc++;
	} // next iNode

	if (Nc<1) return 0;

	en_all = en_all / Nc;
	
	return 1.0f - en_all;
}

// 超解像度版　1回のEGM計算を実行
int C_EGM_FDR::EGM_FDR_SR_Main(int range, int reso)
{
	// 変数宣言と初期化
	// -更新用の格子を記憶する領域
	C_GRID grid_next;
	grid_next.Copy(m_grid);

	// -格子情報
	SDLNODE *node_now = m_grid->GetBuffer();//現在の格子
	SDLNODE *node_next = grid_next.GetBuffer();//更新後の格子

	// -ループカウンタ
	int iNode, iDelta, iLink, i;

	// -探索範囲を格納する配列

	////8近傍最急降下法(森江研)
	//CvPoint deltaxy[] = {
	//	cvPoint( 0, 0),
	//	cvPoint(-1,-1), cvPoint( 0,-1), cvPoint( 1,-1),
	//	cvPoint(-1, 0)                , cvPoint( 1, 0),
	//	cvPoint(-1, 1), cvPoint( 0, 1), cvPoint( 1, 1)
	//};
	//int delta_len = (int)(sizeof(deltaxy) / sizeof(CvPoint));
	// ↓に統合した．range=1で同じ動作になる

	// 矩形領域内で探索
	// 動的配列を利用することに注意
	S_POINT *deltaxy;
	int delta_len;
	{
		int &r = range;//探索範囲
		int &s = reso;
		int rs = r * s;
		int x, y, p = 1;
		delta_len = (2 * rs + 1) * (2 * rs + 1);
		deltaxy = new S_POINT[delta_len];
		deltaxy[0] = s_point(0, 0);//中心は配列の先頭に入れておく

		for ( y = -rs; y <= rs; y++ ) {
			for ( x = -rs; x <= rs; x++) {
				if (x==0 && y==0) continue;
				deltaxy[p] = s_point((float)x/(s * m_width_dy), (float)y/(s * m_height_dy));
				p++;
			}
		}
	}

	// -ループ制御用
	int nodes = m_grid->GetNodeBufferLength();//ノードバッファ長
	int movednodes = 0;// 移動したノードの数

	// 座標参照用
	S_POINT pos_st, pos_dy;
	//S_POINT s_pos_dy;

	// データ参照用
	//int idx_st, idx_dy;

	// エネルギ計算用
	EGM_VALUETYPE norm_dy, norm_st;
	EGM_VALUETYPE en_jetsim, en_jetdist, en_node, en_all = 0;
	EGM_VALUETYPE en_node_winner;
	const EGM_VALUETYPE const_unti_vibration = 0;
	int idx_winner;

	float w_dy_per_st = (float)m_width_dy / m_width_st;
	float h_dy_per_st = (float)m_height_dy / m_height_st;

	// 全てのノードについてループ
	for (iNode = 0; iNode < nodes; iNode ++) {

		if (node_now[iNode].active == FALSE) continue;

		// 現在注目しているノードのstatic側座標を取得
		//pos_st = S_POINTtoCvPoint(node_now[iNode].pos_st, m_width_st, m_height_st);
		pos_st = node_now[iNode].pos_st;

		if(pos_st.x < 0) continue;
		if(pos_st.x >= m_width_st)  continue;
		if(pos_st.y < 0) continue;
		if(pos_st.y >= m_height_st) continue;

		//idx_st = (pos_st.y * m_width_st + pos_st.x) * m_jetdim;

		// 最大値探索用変数の初期化
		en_node_winner = node_now[iNode].value;//前回の計算の結果を初期値にする
		//en_node_winner = 0;
		idx_winner = 0;

		// dynamic側の注目座標を変えながらエネルギ最大の点を探す
		for(iDelta = 0; iDelta < delta_len; iDelta++) {

			// 現在注目しているノードのdynamic側座標を取得
			//pos_dy = deltaxy[iDelta] + S_POINTtoCvPoint(node_now[iNode].pos_dy, m_width_dy, m_height_dy);
			//s_pos_dy = CvPointtoS_POINT(pos_dy, m_width_dy, m_height_dy);
			pos_dy = deltaxy[iDelta] + node_now[iNode].pos_dy;

			if(pos_dy.x < 0) continue;
			if(pos_dy.x >= m_width_dy)  continue;
			if(pos_dy.y < 0) continue;
			if(pos_dy.y >= m_height_dy) continue;
				
			//idx_dy = (pos_dy.y * m_width_dy + pos_dy.x) * m_jetdim;

			// Jetの内積を求める -> en_jetsim
			norm_st = 0;
			norm_dy = 0;
			en_jetsim = 0;

			if ( m_jetdim > 1 ) {
				{
					float jet_st, jet_dy;
					for(i = 0; i < m_jetdim; i ++) {
						jet_st = GetJet_st(pos_st, i);
						jet_dy = GetJet_dy(pos_dy, i);
						en_jetsim += jet_st * jet_dy;
						norm_dy += jet_dy * jet_dy;
						norm_st += jet_st * jet_st;
						//en_jetsim += m_jet_st[idx_st + i] * m_jet_dy[idx_dy + i];
						//norm_dy += m_jet_dy[idx_dy + i] * m_jet_dy[idx_dy + i];
						//norm_st += m_jet_st[idx_st + i] * m_jet_st[idx_st + i];
					}
				}
				norm_dy = EGM_SQRT(norm_dy);
				norm_st = EGM_SQRT(norm_st);

				// 分母が0になるようだったら内積の最低値(-1)にする
				if( norm_dy == 0 || norm_st == 0)
					en_jetsim = 0;
				else
					en_jetsim = en_jetsim / (norm_st * norm_dy);

				//減算項
				//ノルムの差をとって大きいほうのノルムで割る
				//float diff = fabs(norm_dy - norm_st) / ((norm_dy>norm_st)?norm_dy:norm_st);
				//en_jetsim -= diff * m_weight_diff;

			} else {
				en_jetsim = fabs(GetJet_st(pos_st, 0) * GetJet_dy(pos_dy, 0));
			}
			// 格子の歪みを計算 -> en_jetdist
			en_jetdist = 0;

			// st-dy間のリンク長の差　|固定側リンク - 移動側リンク|/固定側リンク
#ifdef EGMD_LINKLENGTH
			{
				//リンクの数を数える
				int Lc=0;
				EGM_VALUETYPE v=0;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//リンク先のインデックス
					S_POINT vLink_dy = s_point(
						(node_now[n].pos_dy.x - pos_dy.x),
						(node_now[n].pos_dy.y - pos_dy.y)
					);
					//S_POINT vLink_dy = s_point(
					//	(node_now[n].pos_dy.x - pos_dy.x) * w_dy_per_st,
					//	(node_now[n].pos_dy.y - pos_dy.y) * h_dy_per_st
					//);
						S_POINT vLink_st = s_point(
						node_now[n].pos_st.x - node_now[iNode].pos_st.x,
						node_now[n].pos_st.y - node_now[iNode].pos_st.y
					);

					EGM_VALUETYPE f = s_size(vLink_st - vLink_dy) / s_size(vLink_st);
					//v += fabs(f);
					v += f * f;

					Lc++;//リンクの数を数えておく
				}

				// 歪み度 = 
				en_jetdist += v / Lc;
			}
#endif
#ifdef EGMD_TYPEM
			// M式改：リンク長の分散=ave(|リンク長の平均-リンク長|/リンク長の平均)
			{
				//リンクの数を数える
				int Lc=0;
				EGM_VALUETYPE ave=0, v=0;
				int n;

				// リンク長の平均を求める
				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;
					n = node_now[iNode].link[iLink];//リンク先のインデックス

					S_POINT vLink_dy = node_now[n].pos_dy - pos_dy;
					ave += s_size(vLink_dy);
					Lc++;//リンクの数を数えておく
				}
				if (Lc == 0) break;
				ave /= Lc;
				if (ave == 0) break;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					S_POINT vLink_dy = node_now[n].pos_dy - pos_dy;
					EGM_VALUETYPE f = ave - s_size(vLink_dy);
					v += EGM_SQRT(f * f);
					//EGM_VALUETYPE f = fabs(ave - s_size(vLink_dy)/ave);
					//v += f;// * f;
				}

				// 歪み度 = 
				en_jetdist += v / Lc;
			}
#endif
#ifdef EGMD_LINKDOT
			// 案２'：リンクのベクトルをst, dy側それぞれで作り
			// 1-MIN(内積)を歪み度とする
			{
				//リンクの数を数える
				int Lc=0;
				EGM_VALUETYPE v=1.0f;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//リンク先のインデックス
					S_POINT vLink_dy = node_now[n].pos_dy - pos_dy;
					S_POINT vLink_st = node_now[n].pos_st - node_now[iNode].pos_st;

					// 内積をとる
					EGM_VALUETYPE f = vLink_dy * vLink_st;

					if (f<v) v=f;

					Lc++;//リンクの数を数えておく
				}

				// 歪み度 = 1 - 内積の平均
				//en_jetdist += 1.0f - v;
				en_jetdist += 1.0f - v;

			}
#endif
			// 案１：格子の移動距離　|固定側座標 - 移動側座標|
			//{
			//	en_jetdist += s_size(node_now[iNode].pos_st - s_pos_dy);
			//}
	
			// 重みをつけてノードのエネルギを計算 -> en_node
			en_node = m_weight_sim * en_jetsim - m_weight_dist * en_jetdist * EGMD_CONST;

			// 最大値をとったエネルギを記憶
//			if(en_node > en_node_winner + const_unti_vibration){
			if(en_node > en_node_winner + const_unti_vibration || iDelta == 0 ){
				en_node_winner = en_node;
				idx_winner = iDelta;
			}

			// ノードに固定属性がついているときは
			// ループせずに抜ける(ループの１回目が移動しない場合のエネルギ計算)
			if (node_now[iNode].lock == TRUE) break;
			//if (norm_st < m_th_lock)
			//	node_next[iNode].lock = TRUE;

			//if (node_next[iNode].lock)
			//	break;

		} // next iDelta

		// 他のノードと衝突したときの処理をするときはここに書く

		// エネルギが最大になる点が中心でなかったら 格子を移動する
		if ( idx_winner != 0 ) {
			//node_now[iNode].pos_dyi = deltaxy[idx_winner] + node_now[iNode].pos_dyi;
			node_next[iNode].pos_dy = deltaxy[idx_winner] + node_now[iNode].pos_dy;
				//CvPointtoS_POINT(
				//						deltaxy[idx_winner]
				//						+ S_POINTtoCvPoint(node_now[iNode].pos_dy, m_width_dy, m_height_dy)
				//						, m_width_dy
				//						, m_height_dy);
			movednodes ++;
		}

		// エネルギを記憶
		node_next[iNode].value = en_node_winner;

		// このノードのエネルギを全体のエネルギに加算
		en_all += en_node_winner / m_grid->GetActiveNodeCount();
//		en_all += en_node_winner * node_now[iNode].weight;

	} // next iNode

	// ノードの移動をメンバ変数に反映
	m_grid->Copy(&grid_next);

	//// 歪み度を計算し，エネルギから差し引く
	//for (iNode = 0; iNode < nodes; iNode ++) {

	//	if (node_now[iNode].active == FALSE) continue;
	//	if (node_now[iNode].lock == TRUE) continue;//端っこのノードは減算しない

	//	// 現在注目しているノードのstatic側座標を取得
	//	pos_dy = node_now[iNode].pos_dy;

	//	EGM_VALUETYPE xsum=0, ysum=0;
	//	int n;
	//	for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
	//		n = node_now[iNode].link[iLink];
	//		if (n < 0) continue;
	//		xsum += node_now[n].pos_dy.x - pos_dy.x;
	//		ysum += node_now[n].pos_dy.y - pos_dy.y;
	//	}
	//	en_all -= m_weight_dist * EGM_SQRT(xsum * xsum + ysum * ysum);
	//}

	// エネルギを最大1.0になるように規格化
	// ただし，0で割らないようにする
	m_value = en_all;

	//int c = m_grid->GetActiveNodeCount();
	//if (c > 0)
	//	m_value /= c;

	//if (m_weight_sim != 0)
	//	m_value /= m_weight_sim;

	m_lastmovednodes = movednodes;

	// 動的配列の解放
	delete [] deltaxy;

	// 移動したノードの数を返す
	return movednodes;
}

namespace FDR{
	static int wdisp=1;
}

// ノードの重みを求める
// typeが0なら均一，1ならノルム/(Σノルム)
int C_EGM_FDR::SetNodeWeight(int type) {

	SDLNODE *grid = m_grid->GetBuffer();
	int gridlen = m_grid->GetNodeBufferLength();

	if (type==0) {
		int activecount=0;
		for (int i=0; i<gridlen; i++) {
			if ( ! grid[i].active) {
				grid[i].weight = 0;
				continue;
			}
			activecount++;
		}
		if (activecount==0) return FALSE;

		float w = 1.0f / activecount;

		// 合計で割る
		for (int i=0; i<gridlen; i++) {
			if ( ! grid[i].active) continue;
			grid[i].weight = w;
		}	

	} else {

	//	int activecount=0;
		float sum=0;

		// Jetの合計を計算しながら重みにノルムの値を設定
		for (int i=0; i<gridlen; i++) {
			if ( ! grid[i].active) {
				grid[i].weight = 0;
				continue;
			}

			float ssum = 0;
			int idx = ((int)(grid[i].pos_st.y * m_height_st * m_width_st) + (int)(grid[i].pos_st.x * m_width_st)) * m_jetdim;
			for (int j=0; j<m_jetdim;j++) {
				ssum += m_jet_st[idx + j] * m_jet_st[idx + j];
			}
			ssum = EGM_SQRT(ssum);
			grid[i].weight = ssum;

			sum += ssum;
	//		activecount++;
		}

	//	if (activecount==0) return FALSE;
		if (sum==0) return FALSE;

		// 合計で割る
		for (int i=0; i<gridlen; i++) {
			if ( ! grid[i].active) continue;
			grid[i].weight /= sum;
		}	
		if (FDR::wdisp) {
			printf("weight:");
			for (int i=0; i<gridlen; i++) {
				printf("%.03f ", grid[i].weight);
			}
			printf("\n\n");
			FDR::wdisp=0;
		}
	}	
	return TRUE;
}

namespace FDR {
// 入力画像を正方形の8ビットグレースケールに整形する
void NormaliseImg(IplImage *in, IplImage **out, int w, int h) {
	IplImage *imgtmp=NULL;
	
	// フォーマット変換 (GRAY8)
	if ( in->nChannels == 3 ) {
		imgtmp = cvCreateImage(cvSize(in->width, in->height), IPL_DEPTH_8U, 1);
		cvCvtColor(in, imgtmp, CV_BGR2GRAY);
	} else
		imgtmp = cvCloneImage(in);
	
	if (w == h) {
		// 正方形になるように切り抜き
		if (imgtmp->height > imgtmp->width) {//縦長
			//切抜きでは都合が悪いので余白を加える
			IplImage *imgtmp2 = cvCreateImage(cvSize(in->height,  in->height), IPL_DEPTH_8U, 1);
			int x1 = (imgtmp->height - imgtmp->width) / 2;
			cvSetImageROI(imgtmp2, cvRect(x1, 0, imgtmp->width, imgtmp->height));
			cvCopy(imgtmp, imgtmp2);
			cvResetImageROI(imgtmp2);
			//両端を引き伸ばして埋める
			cvSetImageROI(imgtmp, cvRect(0, 0, 1, imgtmp->height));
			cvSetImageROI(imgtmp2, cvRect(0, 0, x1, imgtmp->height));
			cvResize(imgtmp, imgtmp2, 0);
			cvResetImageROI(imgtmp);
			cvResetImageROI(imgtmp2);

			cvSetImageROI(imgtmp, cvRect(imgtmp->width-2, 0, 1, imgtmp->height));
			cvSetImageROI(imgtmp2, cvRect(imgtmp2->width-x1-1, 0, x1, imgtmp->height));
			cvResize(imgtmp, imgtmp2, 0);
			cvResetImageROI(imgtmp);
			cvResetImageROI(imgtmp2);

			cvReleaseImage(&imgtmp);
			//cvShowImage("debug", imgtmp2);
			//cvWaitKey(0);

			imgtmp = imgtmp2;
		} else if (imgtmp->width > imgtmp->height) {//横長
			int x1 = (imgtmp->width - imgtmp->height) / 2;
			cvSetImageROI(imgtmp, cvRect(x1, 0, imgtmp->height, imgtmp->height));
		}
	}
	// スケーリング
	*out = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);
	cvResize(imgtmp, *out, CV_INTER_AREA);

	cvReleaseImage(&imgtmp);
}
}
