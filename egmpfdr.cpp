#include "stdafx.h"
#include "egm.h"
#include "egmpfdr.h"

#define _USE_MATH_DEFINES
#include <math.h>

//�t�@�C���o�͐�
#define PATH_OUTPUT "c:\\project\\result"

#define EGMD_CONST (1.0f/2)	// �c�ݓx�ɂ���ɏ�Z����l
#define EGMD_LINKLENGTH // st-dy�Ԃ̃����N���̍�
//#define EGMD_TYPEM		// �����N���̕��U
#define EGMD_LINKDOT	// st-dy�Ԃ̃����N�̓���

namespace FDR {
void NormaliseImg(IplImage *in, IplImage **out, int w, int h);
void CutLowerValue(IplImage *in, float th);
}

// ����JET�̏��� th�� �������l
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


// �猟�o�N���X �R���X�g���N�^
//-�I�v�V�����̏����ݒ�Ɠ��I�ϐ��|�C���^�̏�����
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

//�f�X�g���N�^ �m�ۂ�����ςȂ��̓��I�ϐ������
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

// �O����
int C_FACE_DETECT_RECOG::PreProcess(char **file_model)
{
	IplImage *img_tmp;

	// �L���摜�̐���
	m_img_mo = new IplImage*[m_layers_mo*m_models];
	m_jet_mo = new IplImage*[m_layers_mo*m_gabor_ori*m_models];
	m_enmap = new IplImage*[m_layers_mo];
	memset(m_enmap, 0, sizeof(IplImage*)*m_layers_mo);//�g���Ƃ͌���Ȃ��̂�0�N���A���Ă���

	// ��ʕ\���̏���
	if (m_show_filter) {
		RELEASE_IPLIMAGE(m_jet_in_disp);
		int w = m_width_in[0];
		m_jet_in_disp = cvCreateImage(cvSize(w, w*m_gabor_ori), IPL_DEPTH_8U, 3);
		cvSetZero(m_jet_in_disp);
		cvNamedWindow("jet_in");
		cvShowImage("jet_in", m_jet_in_disp);
	}

	// �K�{�[���t�B���^����
	m_gabor = new CvGabor[m_gabor_ori];
	double d;
	for (int r = 0; r < m_gabor_ori; r++) {
		d = M_PI * ((double)r / (double)m_gabor_ori);
		m_gabor[r].Init(d, m_Nu, m_Sigma, m_F);
	}

	// �L���摜�̓ǂݍ��݂ƃK�{�[���ϊ�
	for (int mi = 0; mi < m_models; mi++) {

		// �L���摜�̓ǂݍ���
		img_tmp = cvLoadImage( file_model[mi] );
		if (img_tmp == NULL) {
			return FALSE;
		}

		// �L���摜�̐��`
		for (int m = 0; m < m_layers_mo; m++) {
			int idx = m + mi * m_layers_mo;
			FDR::NormaliseImg(img_tmp, &(m_img_mo[idx]), m_width_mo[m], m_width_mo[m]);

			//�q�X�g�O�����ψꉻ(�L���摜�̔w�i������������Ə�������Ă���K�v������)
			cvEqualizeHist (m_img_mo[idx], m_img_mo[idx]);
		}
		cvReleaseImage(&img_tmp);

		// �L���摜�̃K�{�[���ϊ�
		for (int m=0; m < m_layers_mo; m++) {
			for (int r=0; r < m_gabor_ori; r++) {
				int idx_img = m + mi * m_layers_mo;
				int idx_jet = r + idx_img * m_gabor_ori;
				m_jet_mo[idx_jet] = cvCreateImage(cvSize(m_img_mo[m]->width, m_img_mo[m]->height), IPL_DEPTH_32F, 1);
				m_gabor[r].conv_img(m_img_mo[idx_img], m_jet_mo[idx_jet], CV_GABOR_MAG);
				////����ȉ����̏���
				//FDR::CutLowerValue(m_jet_mo[m * m_gabor_ori + r], 0.1f);
			}
		}
	}

	// jet�̉�ʕ\��
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
					cvNormalize(imgtmp, imgtmp2, 255.0, 0.0, CV_MINMAX);//255~0�ɐ��K��
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

		// model�̉�ʕ\��(�i�q��)
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

// �ɑ�l�̏d����ԏ���
int C_FACE_DETECT_RECOG::PeakElimination(S_PEAK **p_listtop, float th_ov)
{
	int cnt=0;//�������ꂽ�����J�E���g
	int dx, dy, dist;
	S_PEAK *p_top = *p_listtop;
	S_PEAK *p_prev = NULL;
	S_PEAK *p = p_top;
	S_PEAK *ppp;//�ꎞ�ޔ�p�_�~�[
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
						//pp��������
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
						//p��������
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

// �ɑ�l�T��
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
			if (data[p] > th_v)//�������l����
			//if (data[p-ws] > 0)//�[����������
			//if (data[p-1] > 0)
			//if (data[p+1] > 0)
			//if (data[p+ws] > 0)
			if (data[p] > data[p-ws-1])//8�ߖT�Ɣ�r
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
				x++;//�E�ׂ̓s�[�N�łȂ��킯������P��΂�
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

// ���X�g�̉��
void C_FACE_DETECT_RECOG::DeleteS_PEAK(S_PEAK *peak)
{
	while(peak) {
		S_PEAK *p = peak;
		peak = peak->next;
		if (p->grid) delete p->grid;
		delete p;
	}
}

// EGM�v�Z
float C_FACE_DETECT_RECOG::FaceRecognition(IplImage *imgin, int wi, C_GRID **grid_r, int display)
{
	float v;
	IplImage **jet_in = new IplImage*[m_gabor_ori];

	//-���͉摜�̃K�{�[���t�B���^
	for (int r = 0; r < m_gabor_ori; r++) {
		jet_in[r] = cvCreateImage(cvGetSize(imgin), IPL_DEPTH_32F, 1);
		m_gabor[r].conv_img(imgin, jet_in[r], CV_GABOR_MAG);
		////--����ȉ����̏���
		//if (m_filter_noise)
		//	FDR::CutLowerValue(jet_in[r], 0.2f);
	}

	//��ʕ\���̏���
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

		//-egm class�̏���
		C_GRID *grid = new C_GRID;
		grid->CreateRoundCornerSquareGrid(&m_grid_i, m_grid_res, m_grid_res, &m_grid_m);

		C_EGM_FDR* egm = new C_EGM_FDR;
		egm->SetSuperReso(FALSE);
		egm->SetParam(1, m_weight_dist[0]);
		egm->SetJet_st(m_jet_mo + idx_jet_mo, m_gabor_ori);
		egm->SetJet_dy(jet_in, m_gabor_ori);
		egm->SetGrid(grid);
		egm->SetNodeWeight(0);

		//��ʕ\���̏���
		if (display>0) {
			//�L����Ɋi�q�𒣂�t����
			IplImage *img_tmp = cvCreateImage(cvGetSize(m_img_mo[idx_img_mo]), IPL_DEPTH_8U, 3);
			cvCvtColor(m_img_mo[idx_img_mo], img_tmp, CV_GRAY2BGR);
			grid->DrawGrid(img_tmp, FALSE, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 2);//�i�q�`��
			cvSetImageROI(img_disp, cvRect(imgin->width, 0, img_tmp->width, img_tmp->height));
			cvCopy(img_tmp, img_disp);
			cvResetImageROI(img_disp);
			cvReleaseImage(&img_tmp);
		}

		for (int t = 0; t < m_egm_times[0]; t++) {
			int r = egm->EGM_FDR(1, m_egm_range[0]);

			if (display>=2 || grid_r)egm->GetGrid(grid);
			
			//�r���o�߂̉�ʕ\��
			if (display>=2){

				//-���͉摜�Ɋi�q��\��
				IplImage *img_tmp = cvCreateImage(cvSize(imgin->width, imgin->height), IPL_DEPTH_8U, 3);
				cvCvtColor(imgin, img_tmp, CV_GRAY2BGR);
				grid->DrawGrid(img_tmp, TRUE, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 1);//�i�q�`��
				cvSetImageROI(img_disp, cvRect(0, 0, img_tmp->width, img_tmp->height));
				cvCopy(img_tmp, img_disp);
				cvResetImageROI(img_disp);
				cvReleaseImage(&img_tmp);

				//�����`��
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

		if (display==3) { //�t�@�C���o��
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
			delete grid;//grid_r�ɒl�������
	}

	for (int r = 0; r < m_gabor_ori; r ++)
		if (jet_in[r]) cvReleaseImage(jet_in + r);
	delete [] jet_in;

	if (img_disp) {
		cvReleaseImage(&img_disp);
	}
	//�������s�������f�����D��������G�l���M0��Ԃ�
	if (m_modeltype[besti] != 0) {
		bestv = 0;
	}

	return bestv;
}

// �猟�o
int C_FACE_DETECT_RECOG::FaceDetection(IplImage *imgin, S_PEAK **ret, int display, int store_enMap)
{
	int pyra_layers = m_layers_in;
	IplImage *img_in_0;		//���K���ς݃I���W�i���X�P�[�����͉摜
	IplImage *img_in=NULL;
	IplImage **jet_in = new IplImage*[m_gabor_ori];
	memset(jet_in, 0, sizeof(IplImage*)*m_gabor_ori);

	CvFont font;
	char str_tmp[1024];
	if (display>0)
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);

	// �ő�X�P�[���̓��͉摜�쐬
	// ������k�����Ďg��
	int w0 = ((imgin->width > imgin->height)?imgin->height:imgin->width);
	FDR::NormaliseImg(imgin, &img_in_0, w0, w0);
//	FDR::NormaliseImg(imgin, &img_in_0, m_width_in[m_layers_in-1], m_width_in[m_layers_in-1]);

	//EGM_VALUETYPE *result_value = new EGM_VALUETYPE[pyra_layers];
	//S_RECT *result_rect = new S_RECT[pyra_layers];

	// ��ʕ\���̏���
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

	// �i�q�̏����ݒ�
	//S_RECT r_stack = s_rect(0,0,1,1);//�؂蔲���̈�

	int bestm = 0;

	//�猟�o

	//-���͉摜�̃_�E���T���v�����O
	img_in = cvCreateImage(cvSize(m_width_in[0], m_width_in[0]), IPL_DEPTH_8U, 1);
	cvResize(img_in_0, img_in);

	//-���͉摜�̃K�{�[���t�B���^
	for (int r = 0; r < m_gabor_ori; r++) {
		if (jet_in[r] != NULL) cvReleaseImage(jet_in + r);
	}
	for (int r = 0; r < m_gabor_ori; r++) {
		jet_in[r] = cvCreateImage(cvSize(img_in->width, img_in->height), IPL_DEPTH_32F, 1);
		m_gabor[r].conv_img(img_in, jet_in[r], CV_GABOR_MAG);
		//--����ȉ����̏���
		if (m_filter_noise)
			FDR::CutLowerValue(jet_in[r], m_th_weak_feature);
	}
	// jet�̉�ʕ\��
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
		if (display==3) { //�t�@�C���o��
			char fn[MAX_PATH];
			sprintf_s(fn, MAX_PATH, "%s\\jet_in.png", PATH_OUTPUT, m_jet_in_disp);
			cvSaveImage(fn, m_jet_in_disp);
			m_egmcnt++;
		}
	}

	//-���ϋL����őS��T���}�b�`���O
	//-�L���摜�X�P�[���Ń��[�v
	S_PEAK *peak = NULL;
	int peaks = 0;
	for (int m = 0; m < m_layers_mo; m ++) {
		int idx_jet_mo = m * m_gabor_ori;

		//-egm class�̏���
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
		
		//-�S��T��
		IplImage *img_corr;
		egm->SweepFinding(0, &img_corr, ((m_sweepfinding>0)?m_sweepfinding:1));

		//-�s�[�N�T��
		S_PEAK *peak_local=NULL, *peak_last=NULL;
		int peaks_local = PeakDetection(img_corr, m_width_mo[m], 0.77f, 0.5f, &peak_local, &peak_last);

		//�r���o�߂̉�ʕ\��
		if (display>0){

			egm->GetGrid(grid);
			cvSetZero(img_disp);
			
			//-���͉摜��\��t��
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
			
			//���̈��`��
			//grid->DrawGrid(img_tmp2, TRUE, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 1);//�i�q�`��
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

			//-�L���摜��\��t��
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
			grid->DrawGrid(img_tmp2, FALSE, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 2);//�i�q�`��
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
			//�G�l���M�[�}�b�v�̕`��
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
			//-�L�����Ă���
			if (m_enmap[m] != NULL)
				cvReleaseImage(m_enmap + m);
			m_enmap[m] = cvCloneImage(img_en);

			if (display>0) {
				//�\���I���Cwait
				cvWaitKey(100);
				cvShowImage("SweepFinding", img_en);
				if (display==3) { //�t�@�C���o��
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

		//�e�s�[�N��EGM���s���ĕ]���l���X�V

		////EGM�ŕ]���l���X�V
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

		//���̑w�œ���ꂽ�s�[�N�̃��X�g��S�̂̃��X�g�Ɍ���
		if (peak_last)
			peak_last->next = peak;
		peak = peak_local;
		peaks += peaks_local;

		delete egm;
		delete grid;
		cvReleaseImage(&img_corr);
	}
	
	//-�X�P�[���Ԃœ��ʒu�̌���
	peaks -= PeakElimination(&peak, 0.1f);

	//EGM�ŕ]���l���X�V
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

	//�ŏI����
	peaks -= PeakElimination(&peak, 0.75f);
	
	//paek�̍��W�n�����̃X�P�[���ɕϊ�
	//���łɍő�l�T��������m_r_result, m_v_result�ɑ��
	S_PEAK *pp = peak;
	float vmax=0;
	CvRect rmax = cvRect(0,0,1,1);
	while (pp) {
		if (imgin->width > imgin->height) {//����
			int xmod = (imgin->width - imgin->height) / 2;//�Б��̐؂蔲����[px]
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

	//���ʉ摜��`��(display>0�̏ꍇ�̂�)
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
				pp->grid->DrawGrid(imgTmp, 1, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 1);//�i�q�`��
				cvCopyImage(imgTmp, m_img_result);
				cvResetImageROI(m_img_result);
				cvReleaseImage(&imgTmp);
			}
			pp = pp->next;
		}		
		if (display==3) { //�t�@�C���o��
			char fn[MAX_PATH];
			sprintf_s(fn, MAX_PATH, "%s\\result.png", PATH_OUTPUT);
			cvSaveImage(fn, m_img_result);
		}
	}

	m_r_result = rmax;
	m_v_result = vmax;

	// �ϐ����
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

//�G�l���M�[�}�b�v��1���̉摜�ɂ܂Ƃ߂ĕԂ�
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

// �w���EGM�����s�C���s���ꂽ�񐔂�Ԃ�
int C_EGM_FDR::EGM_FDR(int times, int range, int reso)
{
	int cnt = 0;
	while (cnt < times)
	{
		if (reso > 1) {
			if (EGM_FDR_SR_Main(range, reso) <= 0)
			break; //�G���[�܂��͑S�Ẵm�[�h���s���̂Ƃ��ɐ���
		} else {
			if (EGM_FDR_Main(range) <= 0)
			break; //�G���[�܂��͑S�Ẵm�[�h���s���̂Ƃ��ɐ���
		}
		
		//�U�����m
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

//���𑜓x�v�Z�C�w�肳�ꂽ�����_�����W��Jet���߂�C�Œ葤
float C_EGM_FDR::GetJet_st(S_POINT pos, int dim)
{
	CvPoint ipos = S_POINTtoCvPoint(pos, m_width_st, m_height_st);
	int idx = (ipos.y * m_width_st + ipos.x) * m_jetdim + dim;

	float &a = m_jet_st[idx];
	float &b = m_jet_st[idx+m_jetdim];//�E
	float &c = m_jet_st[idx+m_jetdim * m_width_st];//��
	float &d = m_jet_st[idx+m_jetdim * m_width_st+m_jetdim];//�E��
	
	float x = pos.x * m_width_st - (float)ipos.x;
	float y = pos.y * m_height_st - (float)ipos.y;
	return  ( a * (1 - x) * (1 - y)
			+ b *      x  * (1 - y)
			+ c * (1 - x) *      y
			+ d *      x  *      y
			);
}

//���𑜓x�v�Z�C�w�肳�ꂽ�����_�����W��Jet���߂�C�ړ���
float C_EGM_FDR::GetJet_dy(S_POINT pos, int dim)
{
	CvPoint ipos = S_POINTtoCvPoint(pos, m_width_dy, m_height_dy);
	int idx = (ipos.y * m_width_dy + ipos.x) * m_jetdim + dim;

	float &a = m_jet_dy[idx];
	float &b = m_jet_dy[idx+m_jetdim];//�E
	float &c = m_jet_dy[idx+m_jetdim * m_width_dy];//��
	float &d = m_jet_dy[idx+m_jetdim * m_width_dy+m_jetdim];//�E��
	
	float x = pos.x * m_width_dy - (float)ipos.x;
	float y = pos.y * m_height_dy- (float)ipos.y;
	return  ( a * (1 - x) * (1 - y)
			+ b *      x  * (1 - y)
			+ c * (1 - x) *      y
			+ d *      x  *      y
			);
}

// �S��T���C���𑜓x�v�Z�ݒ�͖��������
// �ő�l���Ƃ������W�ƃG�l���M���N���X���ɋL�������
// img_ret���w�肷��΃G�l���M�}�b�v��Ԃ�
// pitch��for���ɒ��� x += pitch �Ƃ������`�œ���
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

	// -�i�q���
	SDLNODE *pnode = m_grid->GetBuffer();//���݂̊i�q

	m_grid->Render(m_width_st, m_height_st, m_width_dy, m_height_dy);

	int nodes = m_grid->GetNodeBufferLength();//�m�[�h�o�b�t�@��
	int anc = m_grid->GetActiveNodeCount();//�A�N�e�B�u�ȃm�[�h�̐�

	// ��ʕ\���p
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
				
				//���όv�Z�ň�v�x�����߂�
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

				// ���ꂪ0�ɂȂ�悤���������v�x��0�ɂ���
				if( norm_dy == 0 || norm_st == 0)
					sim = 0;
				else
					sim = sim / (norm_st * norm_dy);

				// �d�݂������đS�̂̈�v�x�ɉ��Z
				v += sim * pnode[i].weight;
			}

			// �ő�l��������ʒu�ƃG�l���M���L��
			if (v > maxsim) {
				maxsim = v;
				maxx = x;
				maxy = y;
			}
			
			// �G�l���M��z��ɏ�������
			if (img_ret) {
				buff_ret[x+m_width_st/2 + (y+m_height_st/2) * (img_return->widthStep>>2)] = v;
			}

			// �G�l���M�}�b�v�ɑœ_
			if (display) {
				int iv = (int)(v * 255.0f);
				cvRectangle(img_disp, cvPoint(x+m_width_st/2,y+m_height_st/2), cvPoint(x+m_width_st/2,y+m_height_st/2), N_EGM::GetDisplayColor(iv));
			}
		}
	}

	// �N���X������Ԃ̍X�V
	//-�i�q���ő�n�_�Ɉړ�
	for(int i=0; i < nodes; i++) {
		if ( ! pnode[i].active ) continue;
		pnode[i].pos_dyi.x = maxx + pnode[i].pos_sti.x;
		pnode[i].pos_dyi.y = maxy + pnode[i].pos_sti.y;
	}
	m_grid->UnRender(m_width_st, m_height_st, m_width_dy, m_height_dy);
	m_value = maxsim;
	m_lastmovednodes = m_grid->GetActiveNodeCount();//�S�m�[�h���ړ��������Ƃɂ��Ă���

	if(display) {
		//�ő�n�_���Ƃ������W�́C�e�m�[�h�̈�v�x���Čv�Z
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
			// ���ꂪ0�ɂȂ�悤��������0�ɂ���
			if( norm_dy == 0 || norm_st == 0)
				sim = 0;
			else
				sim = sim / (norm_st * norm_dy);
			pnode[i].value = sim;
		}

		// �G�l���M�}�b�v��ʕ\��, 8�{�Ɋg��
		IplImage *img_tmp = cvCreateImage(cvSize(img_disp->width*8, img_disp->height*8), IPL_DEPTH_8U, 3);
		cvResize(img_disp, img_tmp, 0);
		CvPoint pos = cvPoint((maxx+m_width_st/2)*8-4, (maxy+m_height_st/2)*8-4);
		cvDrawRect(img_tmp, pos, pos + cvPoint(8,8), CV_RGB(0,0,0),1);
		cvShowImage("SweepFinding", img_tmp);
		cvReleaseImage(&img_tmp);

		cvReleaseImage(&img_disp);//�����ŃG�l���M�}�b�v�����
	}

	if (img_ret) {
		*img_ret = img_return;//�G�l���M�[�}�b�v���摜�Ƃ��ă��^�[��
	}

	return m_lastmovednodes;
}

int C_EGM_FDR::EGM_FDR_Main(int range)
{
	// �ϐ��錾�Ə�����
	// -�X�V�p�̊i�q���L������̈�
	C_GRID grid_next;
	grid_next.Copy(m_grid);

	// -�i�q���
	SDLNODE *node_now = m_grid->GetBuffer();//���݂̊i�q
	SDLNODE *node_next = grid_next.GetBuffer();//�X�V��̊i�q

	// -���[�v�J�E���^
	int iNode, iDelta, iLink, i;

	// -�T���͈͂��i�[����z��

	////8�ߖT�ŋ}�~���@(�X�]��)
	//CvPoint deltaxy[] = {
	//	cvPoint( 0, 0),
	//	cvPoint(-1,-1), cvPoint( 0,-1), cvPoint( 1,-1),
	//	cvPoint(-1, 0)                , cvPoint( 1, 0),
	//	cvPoint(-1, 1), cvPoint( 0, 1), cvPoint( 1, 1)
	//};
	//int delta_len = (int)(sizeof(deltaxy) / sizeof(CvPoint));
	// ���ɓ��������Drange=1�œ�������ɂȂ�

	// ��`�̈���ŒT��
	// ���I�z��𗘗p���邱�Ƃɒ���
	CvPoint *deltaxy;
	int delta_len;
	{
		int r = range;//�T���͈�
		int x, y, p = 1;
		delta_len = (2 * r + 1) * (2 * r + 1);
		deltaxy = new CvPoint[delta_len];
		deltaxy[0] = cvPoint(0, 0);//���S�͔z��̐擪�ɓ���Ă���

		for ( y = -r; y <= r; y++ ) {
			for ( x = -r; x <= r; x++) {
				if (x==0 && y==0) continue;
				deltaxy[p] = cvPoint(x, y);
				p++;
			}
		}
	}

	// -���[�v����p
	int nodes = m_grid->GetNodeBufferLength();//�m�[�h�o�b�t�@��
	int movednodes = 0;// �ړ������m�[�h�̐�

	// ���W�Q�Ɨp
	CvPoint pos_st, pos_dy;
	//S_POINT s_pos_dy;

	// �f�[�^�Q�Ɨp
	int idx_st, idx_dy;

	// �G�l���M�v�Z�p
	EGM_VALUETYPE norm_dy, norm_st;
	EGM_VALUETYPE en_jetsim, en_jetdist, en_node, en_all = 0;
	EGM_VALUETYPE en_node_winner;
	const EGM_VALUETYPE const_unti_vibration = 0;
	int idx_winner;

	// �S�Ẵm�[�h�ɂ��ă��[�v
	for (iNode = 0; iNode < nodes; iNode ++) {

		if (node_now[iNode].active == FALSE) continue;

		// ���ݒ��ڂ��Ă���m�[�h��static�����W���擾
		//pos_st = S_POINTtoCvPoint(node_now[iNode].pos_st, m_width_st, m_height_st);
		pos_st = node_now[iNode].pos_sti;

		if(pos_st.x < 0) continue;
		if(pos_st.x >= m_width_st)  continue;
		if(pos_st.y < 0) continue;
		if(pos_st.y >= m_height_st) continue;

		idx_st = (pos_st.y * m_width_st + pos_st.x) * m_jetdim;

		// �ő�l�T���p�ϐ��̏�����
		en_node_winner = node_now[iNode].value;//�O��̌v�Z�̌��ʂ������l�ɂ���
		//en_node_winner = 0;
		idx_winner = 0;

		// dynamic���̒��ڍ��W��ς��Ȃ���G�l���M�ő�̓_��T��
		for(iDelta = 0; iDelta < delta_len; iDelta++) {

			// ���ݒ��ڂ��Ă���m�[�h��dynamic�����W���擾
			//pos_dy = deltaxy[iDelta] + S_POINTtoCvPoint(node_now[iNode].pos_dy, m_width_dy, m_height_dy);
			//s_pos_dy = CvPointtoS_POINT(pos_dy, m_width_dy, m_height_dy);
			pos_dy = deltaxy[iDelta] + node_now[iNode].pos_dyi;

			if(pos_dy.x < 0) continue;
			if(pos_dy.x >= m_width_dy)  continue;
			if(pos_dy.y < 0) continue;
			if(pos_dy.y >= m_height_dy) continue;
				
			idx_dy = (pos_dy.y * m_width_dy + pos_dy.x) * m_jetdim;

			// Jet�̓��ς����߂� -> en_jetsim
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

				// ���ꂪ0�ɂȂ�悤����������ς̍Œ�l(-1)�ɂ���
				if( norm_dy == 0 || norm_st == 0)
					en_jetsim = (EGM_VALUETYPE)0;
				else
					en_jetsim = en_jetsim / (norm_st * norm_dy);

				//���Z��
				//�m�����̍����Ƃ��đ傫���ق��̃m�����Ŋ���
				//float diff = fabs(norm_dy - norm_st) / ((norm_dy>norm_st)?norm_dy:norm_st);
				//en_jetsim -= diff * m_weight_diff;

			} else {
				en_jetsim = 1.0f - fabs(m_jet_st[idx_st] - m_jet_dy[idx_dy]) / 255.0f;
			}
			// �i�q�̘c�݂��v�Z -> en_jetdist
			en_jetdist = 0;

			// st-dy�Ԃ̃����N���̍��@|�Œ葤�����N - �ړ��������N|/�Œ葤�����N
#ifdef EGMD_LINKLENGTH
			{
				//�����N�̐��𐔂���
				int Lc=0;
				EGM_VALUETYPE v=0;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X
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

					Lc++;//�����N�̐��𐔂��Ă���
				}

				// �c�ݓx = 
				en_jetdist += v / Lc;
			}
#endif
#ifdef EGMD_TYPEM
			// M�����F�����N���̕��U=ave(|�����N���̕���-�����N��|/�����N���̕���)
			{
				//�����N�̐��𐔂���
				int Lc=0;
				EGM_VALUETYPE ave=0, v=0;
				int n;

				// �����N���̕��ς����߂�
				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;
					n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X

					CvPoint vLink_dy = node_now[n].pos_dyi - pos_dy;
					ave += cvp_size(vLink_dy);
					Lc++;//�����N�̐��𐔂��Ă���
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

				// �c�ݓx = 
				en_jetdist += v / Lc;
			}
#endif
#ifdef EGMD_LINKDOT
			// �ĂQ'�F�����N�̃x�N�g����st, dy�����ꂼ��ō��
			// 1-MIN(����)��c�ݓx�Ƃ���
			{
				//�����N�̐��𐔂���
				int Lc=0;
				EGM_VALUETYPE v=1.0f;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X
					CvPoint vLink_dy = node_now[n].pos_dyi - pos_dy;
					CvPoint vLink_st = node_now[n].pos_sti - node_now[iNode].pos_sti;

					// ���ς��Ƃ�
					EGM_VALUETYPE f = vLink_dy * vLink_st;

					if (f<v) v=f;

					Lc++;//�����N�̐��𐔂��Ă���
				}

				// �c�ݓx = 1 - ���ς̕���
				//en_jetdist += 1.0f - v;
				en_jetdist += 1.0f - v / Lc;

			}
#endif
			// �ĂP�F�i�q�̈ړ������@|�Œ葤���W - �ړ������W|
			//{
			//	en_jetdist += s_size(node_now[iNode].pos_st - s_pos_dy);
			//}
	
			// �d�݂����ăm�[�h�̃G�l���M���v�Z -> en_node
			en_node = m_weight_sim * en_jetsim - m_weight_dist * en_jetdist * EGMD_CONST;

			// �ő�l���Ƃ����G�l���M���L��
//			if(en_node > en_node_winner + const_unti_vibration){
			if(en_node > en_node_winner + const_unti_vibration || iDelta == 0 ){
				en_node_winner = en_node;
				idx_winner = iDelta;
			}

			// �m�[�h�ɌŒ葮�������Ă���Ƃ���
			// ���[�v�����ɔ�����(���[�v�̂P��ڂ��ړ����Ȃ��ꍇ�̃G�l���M�v�Z)
			if (node_now[iNode].lock == TRUE) break;
			//if (norm_st < m_th_lock)
			//	node_next[iNode].lock = TRUE;

			//if (node_next[iNode].lock)
			//	break;

		} // next iDelta

		// ���̃m�[�h�ƏՓ˂����Ƃ��̏���������Ƃ��͂����ɏ���

		// �G�l���M���ő�ɂȂ�_�����S�łȂ������� �i�q���ړ�����
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

		// �G�l���M���L��
		node_next[iNode].value = en_node_winner;

		// ���̃m�[�h�̃G�l���M��S�̂̃G�l���M�ɉ��Z
		en_all += en_node_winner * node_now[iNode].weight;

	} // next iNode

	// �m�[�h�̈ړ��������o�ϐ��ɔ��f
	m_grid->Copy(&grid_next);

	//// �c�ݓx���v�Z���C�G�l���M���獷������
	//for (iNode = 0; iNode < nodes; iNode ++) {

	//	if (node_now[iNode].active == FALSE) continue;
	//	if (node_now[iNode].lock == TRUE) continue;//�[�����̃m�[�h�͌��Z���Ȃ�

	//	// ���ݒ��ڂ��Ă���m�[�h��static�����W���擾
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

	// �G�l���M���ő�1.0�ɂȂ�悤�ɋK�i��
	// �������C0�Ŋ���Ȃ��悤�ɂ���
	m_value = en_all;

	//int c = m_grid->GetActiveNodeCount();
	//if (c > 0)
	//	m_value /= c;

	//if (m_weight_sim != 0)
	//	m_value /= m_weight_sim;

	m_lastmovednodes = movednodes;

	// ���I�z��̉��
	delete [] deltaxy;

	// �ړ������m�[�h�̐���Ԃ�
	return movednodes;
}


//�c�ݓx�����v�Z���ĕԂ�
EGM_VALUETYPE C_EGM_FDR::GetDistortionValue()
{
	SDLNODE *node_now = m_grid->GetBuffer();//���݂̊i�q
	int nodes = m_grid->GetNodeBufferLength();

	EGM_VALUETYPE en_jetdist = 0, en_all = 0;
	int Nc=0;

	// �S�Ẵm�[�h�ɂ��ă��[�v
	for (int iNode = 0; iNode < nodes; iNode ++) {

		if (node_now[iNode].active == FALSE) continue;

		// st-dy�Ԃ̃����N���̍��@|�Œ葤�����N - �ړ��������N|/�Œ葤�����N
		{
			//�����N�̐��𐔂���
			int Lc=0;
			EGM_VALUETYPE v=0;
			int n;

			for (int iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
				if (node_now[iNode].link[iLink] < 0) continue;

				n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X
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

				Lc++;//�����N�̐��𐔂��Ă���
			}

			// �c�ݓx = 
			en_jetdist += v / Lc;
		}

		// �ĂQ'�F�����N�̃x�N�g����st, dy�����ꂼ��ō��
		// 1-MIN(����)��c�ݓx�Ƃ���
		{
			//�����N�̐��𐔂���
			int Lc=0;
			EGM_VALUETYPE v=1.0f;
			int n;

			for (int iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
				if (node_now[iNode].link[iLink] < 0) continue;

				n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X
				CvPoint vLink_dy = node_now[n].pos_dyi - node_now[iNode].pos_dyi;
				CvPoint vLink_st = node_now[n].pos_sti - node_now[iNode].pos_sti;

				// ���ς��Ƃ�
				EGM_VALUETYPE f = vLink_dy * vLink_st;

				if (f<v) v=f;

				Lc++;//�����N�̐��𐔂��Ă���
			}

			// �c�ݓx = 1 - ���ς̕���
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

// ���𑜓x�Ł@1���EGM�v�Z�����s
int C_EGM_FDR::EGM_FDR_SR_Main(int range, int reso)
{
	// �ϐ��錾�Ə�����
	// -�X�V�p�̊i�q���L������̈�
	C_GRID grid_next;
	grid_next.Copy(m_grid);

	// -�i�q���
	SDLNODE *node_now = m_grid->GetBuffer();//���݂̊i�q
	SDLNODE *node_next = grid_next.GetBuffer();//�X�V��̊i�q

	// -���[�v�J�E���^
	int iNode, iDelta, iLink, i;

	// -�T���͈͂��i�[����z��

	////8�ߖT�ŋ}�~���@(�X�]��)
	//CvPoint deltaxy[] = {
	//	cvPoint( 0, 0),
	//	cvPoint(-1,-1), cvPoint( 0,-1), cvPoint( 1,-1),
	//	cvPoint(-1, 0)                , cvPoint( 1, 0),
	//	cvPoint(-1, 1), cvPoint( 0, 1), cvPoint( 1, 1)
	//};
	//int delta_len = (int)(sizeof(deltaxy) / sizeof(CvPoint));
	// ���ɓ��������Drange=1�œ�������ɂȂ�

	// ��`�̈���ŒT��
	// ���I�z��𗘗p���邱�Ƃɒ���
	S_POINT *deltaxy;
	int delta_len;
	{
		int &r = range;//�T���͈�
		int &s = reso;
		int rs = r * s;
		int x, y, p = 1;
		delta_len = (2 * rs + 1) * (2 * rs + 1);
		deltaxy = new S_POINT[delta_len];
		deltaxy[0] = s_point(0, 0);//���S�͔z��̐擪�ɓ���Ă���

		for ( y = -rs; y <= rs; y++ ) {
			for ( x = -rs; x <= rs; x++) {
				if (x==0 && y==0) continue;
				deltaxy[p] = s_point((float)x/(s * m_width_dy), (float)y/(s * m_height_dy));
				p++;
			}
		}
	}

	// -���[�v����p
	int nodes = m_grid->GetNodeBufferLength();//�m�[�h�o�b�t�@��
	int movednodes = 0;// �ړ������m�[�h�̐�

	// ���W�Q�Ɨp
	S_POINT pos_st, pos_dy;
	//S_POINT s_pos_dy;

	// �f�[�^�Q�Ɨp
	//int idx_st, idx_dy;

	// �G�l���M�v�Z�p
	EGM_VALUETYPE norm_dy, norm_st;
	EGM_VALUETYPE en_jetsim, en_jetdist, en_node, en_all = 0;
	EGM_VALUETYPE en_node_winner;
	const EGM_VALUETYPE const_unti_vibration = 0;
	int idx_winner;

	float w_dy_per_st = (float)m_width_dy / m_width_st;
	float h_dy_per_st = (float)m_height_dy / m_height_st;

	// �S�Ẵm�[�h�ɂ��ă��[�v
	for (iNode = 0; iNode < nodes; iNode ++) {

		if (node_now[iNode].active == FALSE) continue;

		// ���ݒ��ڂ��Ă���m�[�h��static�����W���擾
		//pos_st = S_POINTtoCvPoint(node_now[iNode].pos_st, m_width_st, m_height_st);
		pos_st = node_now[iNode].pos_st;

		if(pos_st.x < 0) continue;
		if(pos_st.x >= m_width_st)  continue;
		if(pos_st.y < 0) continue;
		if(pos_st.y >= m_height_st) continue;

		//idx_st = (pos_st.y * m_width_st + pos_st.x) * m_jetdim;

		// �ő�l�T���p�ϐ��̏�����
		en_node_winner = node_now[iNode].value;//�O��̌v�Z�̌��ʂ������l�ɂ���
		//en_node_winner = 0;
		idx_winner = 0;

		// dynamic���̒��ڍ��W��ς��Ȃ���G�l���M�ő�̓_��T��
		for(iDelta = 0; iDelta < delta_len; iDelta++) {

			// ���ݒ��ڂ��Ă���m�[�h��dynamic�����W���擾
			//pos_dy = deltaxy[iDelta] + S_POINTtoCvPoint(node_now[iNode].pos_dy, m_width_dy, m_height_dy);
			//s_pos_dy = CvPointtoS_POINT(pos_dy, m_width_dy, m_height_dy);
			pos_dy = deltaxy[iDelta] + node_now[iNode].pos_dy;

			if(pos_dy.x < 0) continue;
			if(pos_dy.x >= m_width_dy)  continue;
			if(pos_dy.y < 0) continue;
			if(pos_dy.y >= m_height_dy) continue;
				
			//idx_dy = (pos_dy.y * m_width_dy + pos_dy.x) * m_jetdim;

			// Jet�̓��ς����߂� -> en_jetsim
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

				// ���ꂪ0�ɂȂ�悤����������ς̍Œ�l(-1)�ɂ���
				if( norm_dy == 0 || norm_st == 0)
					en_jetsim = 0;
				else
					en_jetsim = en_jetsim / (norm_st * norm_dy);

				//���Z��
				//�m�����̍����Ƃ��đ傫���ق��̃m�����Ŋ���
				//float diff = fabs(norm_dy - norm_st) / ((norm_dy>norm_st)?norm_dy:norm_st);
				//en_jetsim -= diff * m_weight_diff;

			} else {
				en_jetsim = fabs(GetJet_st(pos_st, 0) * GetJet_dy(pos_dy, 0));
			}
			// �i�q�̘c�݂��v�Z -> en_jetdist
			en_jetdist = 0;

			// st-dy�Ԃ̃����N���̍��@|�Œ葤�����N - �ړ��������N|/�Œ葤�����N
#ifdef EGMD_LINKLENGTH
			{
				//�����N�̐��𐔂���
				int Lc=0;
				EGM_VALUETYPE v=0;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X
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

					Lc++;//�����N�̐��𐔂��Ă���
				}

				// �c�ݓx = 
				en_jetdist += v / Lc;
			}
#endif
#ifdef EGMD_TYPEM
			// M�����F�����N���̕��U=ave(|�����N���̕���-�����N��|/�����N���̕���)
			{
				//�����N�̐��𐔂���
				int Lc=0;
				EGM_VALUETYPE ave=0, v=0;
				int n;

				// �����N���̕��ς����߂�
				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;
					n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X

					S_POINT vLink_dy = node_now[n].pos_dy - pos_dy;
					ave += s_size(vLink_dy);
					Lc++;//�����N�̐��𐔂��Ă���
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

				// �c�ݓx = 
				en_jetdist += v / Lc;
			}
#endif
#ifdef EGMD_LINKDOT
			// �ĂQ'�F�����N�̃x�N�g����st, dy�����ꂼ��ō��
			// 1-MIN(����)��c�ݓx�Ƃ���
			{
				//�����N�̐��𐔂���
				int Lc=0;
				EGM_VALUETYPE v=1.0f;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X
					S_POINT vLink_dy = node_now[n].pos_dy - pos_dy;
					S_POINT vLink_st = node_now[n].pos_st - node_now[iNode].pos_st;

					// ���ς��Ƃ�
					EGM_VALUETYPE f = vLink_dy * vLink_st;

					if (f<v) v=f;

					Lc++;//�����N�̐��𐔂��Ă���
				}

				// �c�ݓx = 1 - ���ς̕���
				//en_jetdist += 1.0f - v;
				en_jetdist += 1.0f - v;

			}
#endif
			// �ĂP�F�i�q�̈ړ������@|�Œ葤���W - �ړ������W|
			//{
			//	en_jetdist += s_size(node_now[iNode].pos_st - s_pos_dy);
			//}
	
			// �d�݂����ăm�[�h�̃G�l���M���v�Z -> en_node
			en_node = m_weight_sim * en_jetsim - m_weight_dist * en_jetdist * EGMD_CONST;

			// �ő�l���Ƃ����G�l���M���L��
//			if(en_node > en_node_winner + const_unti_vibration){
			if(en_node > en_node_winner + const_unti_vibration || iDelta == 0 ){
				en_node_winner = en_node;
				idx_winner = iDelta;
			}

			// �m�[�h�ɌŒ葮�������Ă���Ƃ���
			// ���[�v�����ɔ�����(���[�v�̂P��ڂ��ړ����Ȃ��ꍇ�̃G�l���M�v�Z)
			if (node_now[iNode].lock == TRUE) break;
			//if (norm_st < m_th_lock)
			//	node_next[iNode].lock = TRUE;

			//if (node_next[iNode].lock)
			//	break;

		} // next iDelta

		// ���̃m�[�h�ƏՓ˂����Ƃ��̏���������Ƃ��͂����ɏ���

		// �G�l���M���ő�ɂȂ�_�����S�łȂ������� �i�q���ړ�����
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

		// �G�l���M���L��
		node_next[iNode].value = en_node_winner;

		// ���̃m�[�h�̃G�l���M��S�̂̃G�l���M�ɉ��Z
		en_all += en_node_winner / m_grid->GetActiveNodeCount();
//		en_all += en_node_winner * node_now[iNode].weight;

	} // next iNode

	// �m�[�h�̈ړ��������o�ϐ��ɔ��f
	m_grid->Copy(&grid_next);

	//// �c�ݓx���v�Z���C�G�l���M���獷������
	//for (iNode = 0; iNode < nodes; iNode ++) {

	//	if (node_now[iNode].active == FALSE) continue;
	//	if (node_now[iNode].lock == TRUE) continue;//�[�����̃m�[�h�͌��Z���Ȃ�

	//	// ���ݒ��ڂ��Ă���m�[�h��static�����W���擾
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

	// �G�l���M���ő�1.0�ɂȂ�悤�ɋK�i��
	// �������C0�Ŋ���Ȃ��悤�ɂ���
	m_value = en_all;

	//int c = m_grid->GetActiveNodeCount();
	//if (c > 0)
	//	m_value /= c;

	//if (m_weight_sim != 0)
	//	m_value /= m_weight_sim;

	m_lastmovednodes = movednodes;

	// ���I�z��̉��
	delete [] deltaxy;

	// �ړ������m�[�h�̐���Ԃ�
	return movednodes;
}

namespace FDR{
	static int wdisp=1;
}

// �m�[�h�̏d�݂����߂�
// type��0�Ȃ�ψ�C1�Ȃ�m����/(���m����)
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

		// ���v�Ŋ���
		for (int i=0; i<gridlen; i++) {
			if ( ! grid[i].active) continue;
			grid[i].weight = w;
		}	

	} else {

	//	int activecount=0;
		float sum=0;

		// Jet�̍��v���v�Z���Ȃ���d�݂Ƀm�����̒l��ݒ�
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

		// ���v�Ŋ���
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
// ���͉摜�𐳕��`��8�r�b�g�O���[�X�P�[���ɐ��`����
void NormaliseImg(IplImage *in, IplImage **out, int w, int h) {
	IplImage *imgtmp=NULL;
	
	// �t�H�[�}�b�g�ϊ� (GRAY8)
	if ( in->nChannels == 3 ) {
		imgtmp = cvCreateImage(cvSize(in->width, in->height), IPL_DEPTH_8U, 1);
		cvCvtColor(in, imgtmp, CV_BGR2GRAY);
	} else
		imgtmp = cvCloneImage(in);
	
	if (w == h) {
		// �����`�ɂȂ�悤�ɐ؂蔲��
		if (imgtmp->height > imgtmp->width) {//�c��
			//�ؔ����ł͓s���������̂ŗ]����������
			IplImage *imgtmp2 = cvCreateImage(cvSize(in->height,  in->height), IPL_DEPTH_8U, 1);
			int x1 = (imgtmp->height - imgtmp->width) / 2;
			cvSetImageROI(imgtmp2, cvRect(x1, 0, imgtmp->width, imgtmp->height));
			cvCopy(imgtmp, imgtmp2);
			cvResetImageROI(imgtmp2);
			//���[�������L�΂��Ė��߂�
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
		} else if (imgtmp->width > imgtmp->height) {//����
			int x1 = (imgtmp->width - imgtmp->height) / 2;
			cvSetImageROI(imgtmp, cvRect(x1, 0, imgtmp->height, imgtmp->height));
		}
	}
	// �X�P�[�����O
	*out = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);
	cvResize(imgtmp, *out, CV_INTER_AREA);

	cvReleaseImage(&imgtmp);
}
}
