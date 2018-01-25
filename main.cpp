// EGM�s���~�b�h���C��

#include "stdafx.h"
#include "egm.h"
#include "egmpfdr.h"
#include "ocvfd.h"

#ifndef MAX_PATH
#define MAX_PATH 260 //�p�X�܂ރt�@�C�����̍ő啶����
#endif
int SearchFolder(char *folder, char *filter, char ***path);
void ReleaseSearchFolder(char ***path, int faces);
int ofd(char *fn, char *filter, char *title);

// �r���o��/���ʕ\���E�B���h�E��
#define WN_VIEW "View"
#define WN_RESULT "Result"

//feret�����f�[�^
struct LSFILES {
	char name[MAX_PATH];
	int eyelx;
	int eyely;
	int eyerx;
	int eyery;
};

//caltec�����f�[�^
struct CAFILES {
	CvPoint lb;
	CvPoint lt;
	CvPoint rb;
	CvPoint rt;
	CvRect area;
};

//--setting----------------------------------------------------------------

// BioID�e�X�g�f�[�^�p�X
char path_biod [] =  "BioID-FaceDatabase-V1.2";
int files_bioid = 1521;

//barca
char path_barca_img[] = "C:\\project\\common\\barca\\pics";

//caltec
char path_caltech_img[] = "C:\\project\\common\\caltech";
char path_caltech_tru[] = "C:\\project\\common\\caltech\\truth.txt";

//feret
char path_feret_img[] = "C:\\project\\common\\FERET2001\\original\\images";
char path_feret_tru[] = "C:\\project\\common\\FERET2001\\original\\ground_truths\\name_value";
#define CASCADE_PATH "C:\\OpenCV2.1\\data\\haarcascades\\haarcascade_frontalface_default.xml"

//�摜�f�[�^�̂���t�H���_
char path_data[] = "c:\\project\\common";
char file_in_default[] = "all.png";//�f�t�H���g���͉摜�t�@�C��

//���f���t�@�C���̒�`
int models_default = 1;
char* file_mo_default[] = {"modelfaces\\average_men_germany.jpg"};
int modeltype_default[] = {0};

//-�s�������f����ǉ���������
//int models_default = 2;
//char* file_mo_default[] = {"modelfaces\\average_men_germany.jpg", "objects\\errsato.png"};
//int modeltype_default[] = {0,1};

// �O���[�v���ʗ��e�X�g�p�p�X
char file_mo_rt[] = "3.png";
char path_in_rta[] = "y11";
char path_in_rtb[] = "faces";
char path_in_rtc[] = "objects";
char filter_rta[] = "*.png";
char filter_rtb[] = "*.png";
char filter_rtc[] = "*.bmp";

//�����I�v�V����
//int mode = 0; //0:�P�񏈗�, 1:�J��������, 2:ocv�J��������, 3:ocv-BioID, 4:fdr-BioID
//int display = 2;//�r���o�ߕ\�������邩�ǂ��� 0:���S�ɂȂ� 1:�������� 2:����
int show_filter = TRUE;//�K�{�[�������̕\��
int show_extended = TRUE;//�摜�̊g��\��display=2�̂Ƃ������L��
int superreso = FALSE;//���𑜓x�v�Z
int sweepfinding = 1;//�S��T���C0�Ȃ�I�t�C0���傫���ꍇ�͌v�Z�s�b�`-1[px]

int node_weight = 1; //�m�[�h�d�� 0:�ψ�, 1:�m����

int filter_noise = TRUE; //����ȉ����̏������s�����ǂ���
float th_weak_feature_elim = 5.0f;//����ȉ����̏����������l

//float g_th_camera = 0.763f; //�J�������[�h���̈�v�x�������l
float g_th_camera = 0.785f; //�J�������[�h���̈�v�x�������l

//�L���摜���C���[�̒�`
int layers_fd = 6;								
//int width_fd[] = {60, 52, 45, 37, 30, 26};
//int width_fd[] = {60, 51, 43, 36, 31, 26};
int width_fd[] = {60, 51, 43, 37, 31, 27};		

//���͉摜���C���[�̒�`
int layers = 1;
int width[] = {60};				//^				

//�K�{�[���t�B���^
int	gabor_ori = 4;								//�K�{�[���p�x�̐�
double	Sigma = 1.0 * M_PI;						// �K�E�X�֐��̕W���΍�
double	F = 2;									// ���g��
double	Nu = 0;									// �W���΍��I�t�Z�b�g �ݒ�\�͈�:-5~��

//�i�q�i�O���t�j
S_RECT grid_m = s_rect_2p(0.1f, 0.1f, 0.9f, 0.9f);
S_RECT grid_i = s_rect_2p(0.1f, 0.1f, 0.9f, 0.9f);
int grid_res = 4;//�i�q�̖ڂ̐�

// EGM
EGM_VALUETYPE weight_dist[] = {0.5f, 0.5f, 0.5f}; //�c�ݓx�i���݂͑�1���̂ݎg�p�j
int egm_times[] = {20, 1, 1};					//�v�Z��
int egm_range[] = {2, 2, 2};					//maximum-op.�̒T���͈́i�m�[�h��1��̌v�Z�ł̒T���͈́j

//�����̌v�Z���@�p�i���ݖ��g�p�j
int ffd_reso = 2;
int detect_depth = 3;
int find_reso = 0;
int recog_reso = 2;

//-------------------------------------------------------------------------

// ���C���R�[�h
int _tmain(int argc, _TCHAR* argv[])
{
	int mode=0;
	int display=0;
	char file_in[MAX_PATH];
	char **file_mo = NULL;
	int models = 1;
	int *modeltype=NULL;

	// ���j���[�̕\��
	printf("0: Process Once\n");
	printf("1: Camera Mode\n");
	printf("2: BioID Test\n");
	printf("3: FERET TEST\n");
	printf("4: Caltech TEST\n");
	printf("5: Barca TEST\n");

	printf("11: Haar Camera Mode\n");
	printf("12: Haar BioID \n");
	printf("13: Haar FERET\n");
	printf("14: Haar Caltech\n");
	printf("15: Haar Barca\n");

	printf("Select Mode>");
	scanf_s("%d", &mode);

	if (mode < 0) return 0;
	if (mode > 5 && mode < 11) return 0;
	if (mode > 15) return 0;

	// ��ʕ\�����x���̑I��
	if (mode != 1 && mode != 12) {
		printf("0: None(fastest)\n");
		printf("1: Show Movie\n");
		printf("2: Movie + Wait\n");
		printf("3: Movie + Wait + FileOut\n");
		printf("Select Display Level>");
		scanf_s("%d", &display);
		if (display<0 || display>3) return 0;
	}

	// ��r�p�̓��탂�[�h
	if(mode == 11) {//�r�I�����W���[���Y �J��������
		OCVFD::fdloop();	
		return 0;
	} else if(mode == 12) {//�r�I�����W���[���Y BioID�x���`�}�[�N
		OCVFD::fdbatch(display);
		return 0;
	}

	// ���͉摜�I��, �ǂݍ���
	if (mode == 0) { 
		//���̓t�@�C���w��
		if (ofd(file_in, "All files(*.*)\0*.*\0\0", "���͉摜��I��") == 0) {
			//�L�����Z�����ꂽ��f�t�H���g�ݒ���g�p
			sprintf_s(file_in, MAX_PATH, "%s\\%s", path_data, file_in_default);
		}
	}

	printf("Initializing...\n");

	// ����
	C_FACE_DETECT_RECOG cFR;

	if (mode < 10) {
		// ���f���摜�ǂݍ���
		models = 1;
		modeltype = modeltype_default;
		file_mo = new char* [models];
		for (int i=0; i < models; i++) {
			char *buff = new char[MAX_PATH];
			sprintf_s(buff, MAX_PATH, "%s\\%s", path_data, file_mo_default[i]);
			file_mo[i] = buff;
		}
		cFR.m_stage = 1;
		cFR.m_gabor_ori = gabor_ori;
		cFR.m_show_filter = show_filter;
		cFR.m_width_in = width;
		cFR.m_layers_in = layers;
		cFR.m_width_mo = width_fd;
		cFR.m_layers_mo = layers_fd;
		//cFR.m_ffd_reso = ffd_reso;
		//cFR.m_detect_depth = detect_depth;
		//cFR.m_find_reso = find_reso;
		//cFR.m_recog_reso = recog_reso;
		cFR.m_node_weight = node_weight;
		cFR.m_filter_noise = filter_noise;
		//cFR.m_layers_mo = layers;
		//cFR.m_model_layer_width = model_layer_width;
		//cFR.m_input_width = input_width;
		//cFR.m_input_height = input_height;
		cFR.m_Nu = Nu;
		cFR.m_Sigma = Sigma;
		cFR.m_F = F;
		cFR.m_grid_m = grid_m;
		cFR.m_grid_i = grid_i;
		cFR.m_grid_res = grid_res;
		cFR.m_egm_times = egm_times;
		cFR.m_egm_range = egm_range;
		cFR.m_weight_dist = weight_dist;
//		cFR.m_weight_diff = weight_diff;
		cFR.m_th_weak_feature = th_weak_feature_elim;
		cFR.m_sweepfinding = sweepfinding;
		cFR.m_superreso = superreso;
		cFR.m_show_extended = show_extended;
		cFR.m_models = models;
		cFR.m_modeltype = modeltype;
		strcpy_s(cFR.m_str_view_window_name, MAX_PATH, WN_VIEW);
		if ( cFR.PreProcess(file_mo) == FALSE ) {
			printf("model image not found\n");
			cvWaitKey (0);
			goto er;
		}
	}

	// 1�񏈗����[�h
	if (mode == 0) {

		// ���͉摜�̓ǂݍ���
		unsigned long time_start = timeGetTime();
		IplImage *img_in;
		{
			//-�ǂݍ���
			IplImage *img_tmp = cvLoadImage( file_in );
			if (img_tmp == NULL) {
				printf("input image not found\n");
				cvWaitKey (0);
				goto er;
			}

			//-�t�H�[�}�b�g�ϊ�
			if (img_tmp->nChannels == 3) {
				img_in = cvCreateImage(cvSize(img_tmp->width, img_tmp->height), IPL_DEPTH_8U, 1);
				cvCvtColor(img_tmp, img_in, CV_BGR2GRAY);
			} else {
				img_in = cvCloneImage(img_tmp);
			}

			cvReleaseImage(&img_tmp);
		}

		// �L�[���͑҂�
		if (display>=2) {
			cvNamedWindow(WN_VIEW, 1);
			printf("system ready. press any key on image window.\n");
			cvWaitKey(0);
		}

		IplImage *img_disp;
		char str_tmp[1024];
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
		
		// �猟�o���s
		S_PEAK *p=NULL;
		cFR.FaceDetection(img_in, &p, display);
		
		// ���o���ʂ̃��X�g��p�ɓ����Ă���̂ŉ�ʂɕ`��
		img_disp = cFR.GetResultImage();//display==0�ł͎��s����NULL���Ԃ��Ă���
		if (img_disp == NULL) {
			img_disp = cvCreateImage(cvSize(img_in->width, img_in->height), IPL_DEPTH_8U, 3);
			cvCvtColor(img_in, img_disp, CV_GRAY2BGR);
			S_PEAK *pp = p;
			while (pp) {
				cvDrawRect(img_disp,
					pp->pos - cvPoint(pp->scale/2, pp->scale/2),
					pp->pos + cvPoint(pp->scale/2, pp->scale/2),
					CV_RGB(255,255,0));
				sprintf_s(str_tmp, 1024, "%.3f", pp->value);
				cvPutText(img_disp, str_tmp, pp->pos + cvPoint(-pp->scale/2, pp->scale/2), &font, CV_RGB(255,255,255));
				printf("VALUE=%6.4f POS=(%d %d) WIDTH=%d\n",
					pp->value,
					pp->pos.x,
					pp->pos.y,
					pp->scale
					);
				pp = pp->next;
			}
		}
		// ���X�gp�����
		cFR.DeleteS_PEAK(p);

		// ��ʂɕ\��
		cvShowImage(WN_VIEW, img_disp);
		cvReleaseImage(&img_disp);

		printf("\n\nPress Any Key on Image Window\n");
		cvWaitKey(0);

		// �ϐ����
		cvReleaseImage(&img_in);

	} else if (mode == 2) {
		//BioID�e�X�g

		char str_tmp[1024];
		//�����f�[�^
		int *eye_lx = new int[files_bioid];
		int *eye_ly = new int[files_bioid];
		int *eye_rx = new int[files_bioid];
		int *eye_ry = new int[files_bioid];

		int cnt_success=0;

		//�����f�[�^�ǂݍ���
		for (int i = 0; i < files_bioid; i++) {
			char str1[16];
			int  lx, ly, rx, ry;
			FILE *fp=NULL;
			sprintf_s(str_tmp, 1024, "%s\\%s\\BioID_%04d.eye", path_data, path_biod, i);
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

		cvNamedWindow (WN_VIEW, 1);

		unsigned long time_start = timeGetTime();
		IplImage *img_tmp=NULL, *img_in=NULL;

		printf("success/progress/images\n");
		
		for (int i = 0; i < files_bioid; i++) {
			
			sprintf_s(str_tmp, 1024, "%s\\%s\\BioID_%04d.pgm", path_data, path_biod, i);
			img_tmp = cvLoadImage(str_tmp);

			if (img_tmp==NULL)
				continue;

			//-�t�H�[�}�b�g�ϊ�
			if (img_tmp->nChannels == 3) {
				img_in = cvCreateImage(cvSize(img_tmp->width, img_tmp->height), IPL_DEPTH_8U, 1);
				cvCvtColor(img_tmp, img_in, CV_BGR2GRAY);
			} else {
				img_in = cvCloneImage(img_tmp);
			}

			CvFont font;
			cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
			S_PEAK *p=NULL;

			//-�猟�o
			int ret=0;
			ret = cFR.FaceDetection(img_in, &p, ((display<=1)?0:display));
			
			int flag=FALSE;
			CvRect r;
			if (ret>0) {
				r = cFR.m_r_result;
				// ���𔻒�F���o���ʂ̏�3/5�ɗ��ڂ������Ă��邩�ǂ����`�F�b�N
				if (r.x < eye_lx[i] && r.x+r.width > eye_lx[i] && r.y < eye_ly[i] && r.y+r.height*3/5 > eye_ly[i]
				&&  r.x < eye_rx[i] && r.x+r.width > eye_rx[i] && r.y < eye_ry[i] && r.y+r.height*3/5 > eye_ry[i]) {
					cnt_success ++;
					flag=TRUE;
				}
			}

			// �r���o�ߕ\��
			printf("\r%4d/%4d/%4d %3d%%", cnt_success, i+1, files_bioid, cnt_success*100/(i+1));

			// ��ʕ\��
			if (display>=1) {
				//if (flag)
				if (flag || ret>0) {
					S_PEAK *pp = p;
					while (pp) {
						cvDrawRect(img_in,
							pp->pos - cvPoint(pp->scale/2, pp->scale/2),
							pp->pos + cvPoint(pp->scale/2, pp->scale/2),
							CV_RGB(0,0,0));
						sprintf_s(str_tmp, 1024, "%.3f", pp->value);
						cvPutText(img_in, str_tmp, pp->pos + cvPoint(-pp->scale/2, pp->scale/2), &font, CV_RGB(255,255,255));
						pp = pp->next;
					}
					cvDrawRect(img_in, cvPoint(r.x, r.y), cvPoint(r.x + r.width, r.y + r.height), CV_RGB(255,255,255), 2);
				}

				cvShowImage(WN_VIEW, img_in);
				
				if(flag==FALSE)
					printf(" FAILED:#%d\n", i);
				
				int key;
				if(flag==FALSE && display>=2)
					key = cvWaitKey(0);
				else if(flag==FALSE && display>0)
					key = cvWaitKey(0);
				else
					key = cvWaitKey(10);
				if (key == VK_SPACE)
					cvWaitKey(0);
			}

			if (p) cFR.DeleteS_PEAK(p);
			cvReleaseImage(&img_in);
			img_in = NULL;
			cvReleaseImage(&img_tmp);
		}

		unsigned long time_delta = timeGetTime() - time_start;
		printf("\nTime:%d[ms]\n%d%% %d[ms]", time_delta, cnt_success*100/files_bioid, time_delta/files_bioid);

		delete [] eye_lx;
		delete [] eye_ly;
		delete [] eye_rx;
		delete [] eye_ry;

		cvWaitKey (0);

	} else if (mode == 1){
		//�J�������̓��[�h

		cvNamedWindow(WN_VIEW, 1);

		// �J�����̃Z�b�g�A�b�v
		CvCapture *cap=NULL;
		cap = cvCreateCameraCapture(0);
		cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH, 320);
		cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT, 240);

		int key=0;
		IplImage *img_cap=NULL, *img_in=NULL, *img_out=NULL;
		unsigned long time_start = timeGetTime(), time_now, time_delta;
		char str_tmp[1024];
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
		
		// �ǐՏ����p�ϐ��̒�`
		//-�Ƃ肠����1�����L�����Ēǐ�
		CvPoint trackPos = cvPoint(160, 120);
		int trackEnabled = 0;
		int trackTh = 64;//�����������l

		do {
			// �t���[����荞��
			img_cap = cvQueryFrame(cap);//��荞�񂾃t���[����releaseImage���Ă͂����Ȃ�
			
			// �t�H�[�}�b�g�ϊ�
			if (img_cap->nChannels == 3) {
				img_in = cvCreateImage(cvSize(img_cap->width, img_cap->height), IPL_DEPTH_8U, 1);
				cvCvtColor(img_cap, img_in, CV_BGR2GRAY);
			} else {
				img_in = cvCloneImage(img_cap);
			}
			img_out = cvCreateImage(cvSize(img_cap->width, img_cap->height), IPL_DEPTH_8U, 3);
			cvCvtColor(img_in, img_out, CV_GRAY2BGR);

			// �猟�o
			int ret;
			S_PEAK *p=NULL;
			ret = cFR.FaceDetection(img_in, &p, 0, 1);

			// ���ʕ`��
			int trackBestDist = 320+240;
			CvPoint trackBestPos = trackPos;
			S_PEAK *pp = p;
			while (pp) {
				
				//�����ň�v�x�ɂ������l�������Ă���
				//g_th_camera�����̌��o���ʂ́C�ǐՂ͂��邪�\�����Ȃ�
				if (pp->value > g_th_camera) {
					//�g
					cvDrawRect(img_out,
						pp->pos - cvPoint(pp->scale/2, pp->scale/2),
						pp->pos + cvPoint(pp->scale/2, pp->scale/2),
						CV_RGB(255,255,255),2);
					
					//��v�x
					sprintf_s(str_tmp, 1024, "%.3f", pp->value);
					cvPutText(img_out, str_tmp, pp->pos + cvPoint(-pp->scale/2, pp->scale/2), &font, CV_RGB(255, 0, 0));
					
					//�i�q
					if (pp->grid) {
						int s = pp->scale/2;
						cvSetImageROI(img_out, cvRect(pp->pos.x-s, pp->pos.y-s, pp->scale, pp->scale));
						IplImage *imgTmp = cvCreateImage(cvGetSize(img_out), IPL_DEPTH_8U, 3);
						cvCopyImage(img_out, imgTmp);
						pp->grid->DrawGrid(imgTmp, 1, CV_RGB(255,0,0), CV_RGB(255,255,255), CV_RGB(0,0,255), 1);//�i�q�`��
						cvCopyImage(imgTmp, img_out);
						cvResetImageROI(img_out);
						cvReleaseImage(&imgTmp);
					}
				}

				//�ǐՏ���
				//�������������l�ȓ��ň�ԋ߂����ʂ�ǂ�������
				int d = (int)cvp_size(pp->pos - trackPos);
				if ( d < trackBestDist ) {
					trackBestPos = pp->pos;
					trackBestDist = d;
				}
				pp = pp->next;
			}
			if (p) cFR.DeleteS_PEAK(p);

			//�ǐՏ���
			if (trackEnabled) {
				if ( trackBestDist < trackTh ) {//�ǐՐ�������
					//���`��
					cvLine(img_out, trackPos, trackBestPos, CV_RGB(255, 255, 0), 2, CV_AA);
					//��ԍX�V
					trackPos = trackBestPos;
				} else {//�����o�܂��͉���������ǐՂ���߂�
					trackEnabled = FALSE;
					trackPos = cvPoint(160, 120);
				}
			} else {
				if (p != NULL) {//�������o���ꂽ��ǐՊJ�n
					trackPos = trackBestPos;
					trackEnabled = TRUE;
				}
			}

			// ���Ԍv��
			time_now = timeGetTime();
			time_delta = time_now - time_start;
			time_start = time_now;

			// ��ʕ\��
			sprintf_s(str_tmp, 1024, "%dms", time_delta);
			cvPutText(img_out, str_tmp, cvPoint(4, 16+4), &font, CV_RGB(255,255,255)); 
			cvShowImage(WN_VIEW, img_out);

			// ��v�x�}�b�v�`��
			IplImage *imgEn = cFR.GetEnergyMap();
			if (imgEn) {
				cvShowImage("EnergyMap", imgEn);
				cvReleaseImage(&imgEn);
			}

			key = cvWaitKey(1);

			//�L�[���͏���
			//-C:�L����X�V
			//if (key == 'c') {
			//	IplImage *imgtmp = cvCreateImage(cvSize(cFR.m_r_result.width, cFR.m_r_result.height), img_in->depth, img_in->nChannels);
			//	char strtmp[MAX_PATH];
			//	cvSetImageROI(img_in, cFR.m_r_result);
			//	cvCopyImage(img_in, imgtmp);
			//	cvResetImageROI(img_in);
			//	
			//	sprintf_s(strtmp, MAX_PATH, "data\\img%d.bmp", GetTickCount());
			//	cvSaveImage(strtmp, imgtmp);
			//	cvReleaseImage(&imgtmp);
			//}

			cvReleaseImage(&img_in);
			cvReleaseImage(&img_out);
		} while ( key != '\x1b' );

		cvReleaseCapture(&cap);
	} else if (mode == 3 || mode == 13) {		//FERET TEST

		char str_tmp[1024];
		int valid_files=0;
		char **str_arr_filenames=NULL;
		int files = SearchFolder(path_feret_tru, "*.gnd", &str_arr_filenames);
		printf("%d files\n", files);

		//cv
		const char *cascade_name = CASCADE_PATH;
		CvHaarClassifierCascade *cascade = 0;
		CvMemStorage *storage = 0;
		CvSeq *seq_faces;
		storage = cvCreateMemStorage (0);
		cascade = (CvHaarClassifierCascade *) cvLoad (cascade_name, 0, 0, 0);
		if (cascade==NULL)
			return 0;

		LSFILES *truth = new LSFILES[files];
		ZeroMemory(truth, sizeof(LSFILES)*files);
		for (int i = 0; i < files; i++) {
			FILE* fp=NULL;
			char fn[MAX_PATH];
			sprintf_s(fn, MAX_PATH, "%s", str_arr_filenames[i]);
			fopen_s(&fp, fn, "rb");
			if (fp==NULL) {
				printf("OPEN ERROR : %s\n", fn);
				continue;
			}
			fn[strlen(fn)-4] = '\0';//�g���q������
			sprintf_s(truth[i].name, MAX_PATH, "%s%s.tif",  path_feret_img, fn + strlen(path_feret_tru));//�摜�t�@�C�����ɕϊ�
			char c;
			int flag1=0,flag2=0;
			while(1) {
				c = fgetc(fp);
				if (c == EOF) break;
				if (c != 0x0A) continue;
				char tag[64];
				int pos=0;
				while(pos<64) {
					c = fgetc(fp);
					if (c == EOF) break;
					if (c == 0x20) break;
					tag[pos] = (char)c;
					pos++;
				}
				tag[pos] = '\0';
				if (strcmp(tag, "left_eye_coords=") == 0) {
					fscanf_s(fp, "%d%d", &truth[i].eyelx, &truth[i].eyely);
					flag1=1;
				} else if (strcmp(tag, "right_eye_coords=") == 0) {
					fscanf_s(fp, "%d%d", &truth[i].eyerx, &truth[i].eyery);
					flag2=1;
				}
			}
			fclose(fp);
			
			if (flag1 && flag2) {
				valid_files++;
				//printf("o");//, i, fn, truth[i].eyelx, truth[i].eyely, truth[i].eyerx, truth[i].eyery);
			//} else {
			//	printf("x");//, i, fn);
			}
		}
		
		printf("\n%d files\n", valid_files);
		
		//��������猟�o

		cvNamedWindow (WN_VIEW, 1);

		unsigned long time_start = timeGetTime();
		IplImage *img_tmp=NULL, *img_in=NULL;

		printf("success/progress/images\n");

		int cnt_success=0;
		int expected_files = valid_files;
		valid_files=0;

		for (int i = 0; i < files; i++) {

			if (truth[i].eyelx==0 && truth[i].eyerx==0 && truth[i].eyely==0 && truth[i].eyery==0) continue;
			
			img_tmp = cvLoadImage(truth[i].name);
			if (img_tmp==NULL)	continue;

			valid_files++;

			//-�t�H�[�}�b�g�ϊ�
			if (img_tmp->nChannels == 3) {
				img_in = cvCreateImage(cvSize(img_tmp->width, img_tmp->height), IPL_DEPTH_8U, 1);
				cvCvtColor(img_tmp, img_in, CV_BGR2GRAY);
			} else {
				img_in = cvCloneImage(img_tmp);
			}
			cvReleaseImage(&img_tmp);
			////�ʐ^�T�C�Y���s�������̂Œ���
			////��25%���J�b�g
			//img_tmp = cvCreateImage(cvSize(img_in->width, img_in->height)), IPL_DEPTH_8U, 1);
			//cvSetImageROI(img_in, cvRect(0, 0, img_in->width, img_in->width));
			//cvCopy(img_in, img_tmp);
			//cvReleaseImage(&img_in);
			//img_in = img_tmp;

			CvFont font;
			cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
			S_PEAK *p=NULL;

			//-�猟�o
			int ret=0;
			int flag=FALSE;
			CvRect r;
			if (mode == 3) {
				
				ret = cFR.FaceDetection(img_in, &p, ((display<=1)?0:display));

				if (ret>0) {
					r = cFR.m_r_result;

					if (r.x < truth[i].eyelx && r.x+r.width > truth[i].eyelx && r.y < truth[i].eyely && r.y+r.height*3/5 > truth[i].eyely
					&&  r.x < truth[i].eyerx && r.x+r.width > truth[i].eyerx && r.y < truth[i].eyery && r.y+r.height*3/5 > truth[i].eyery) {
						cnt_success ++;
						flag=TRUE;
					}
				}

			} else {
				// cv�Ō��o
				cvEqualizeHist (img_in, img_in);
				cvClearMemStorage (storage);
				seq_faces = cvHaarDetectObjects (img_in, cascade, storage, 1.11, 4, 0, cvSize (60, 60));				
				for (int j = 0; j < (seq_faces ? seq_faces->total : 0); j++) {
					CvRect *r = (CvRect *) cvGetSeqElem (seq_faces, j);
					if (r->x < truth[i].eyelx && r->x+r->width > truth[i].eyelx && r->y < truth[i].eyely && r->y+r->height*3/5 > truth[i].eyely
					&&  r->x < truth[i].eyerx && r->x+r->width > truth[i].eyerx && r->y < truth[i].eyery && r->y+r->height*3/5 > truth[i].eyery) {
						if (display>=1) {
							cvDrawRect(img_in, cvPoint(r->x, r->y), cvPoint(r->x + r->width, r->y + r->height), CV_RGB(255,255,255));
						}
						flag=TRUE;
						cnt_success ++;
						break;
					}
				}
			}

			printf("\r%4d/%4d/%4d %3d%%", cnt_success, valid_files, expected_files, cnt_success*100/(valid_files));

			if (display>=1) {
				
				//if (flag)
				if ((flag || ret>0) && mode != 13) {
					S_PEAK *pp = p;
					while (pp) {
						cvDrawRect(img_in,
							pp->pos - cvPoint(pp->scale/2, pp->scale/2),
							pp->pos + cvPoint(pp->scale/2, pp->scale/2),
							CV_RGB(0,0,0));
						sprintf_s(str_tmp, 1024, "%.3f", pp->value);
						cvPutText(img_in, str_tmp, pp->pos + cvPoint(-pp->scale/2, pp->scale/2), &font, CV_RGB(255,255,255));
						pp = pp->next;
					}
					cvDrawRect(img_in, cvPoint(r.x, r.y), cvPoint(r.x + r.width, r.y + r.height), CV_RGB(255,255,255), 2);
				}

				cvShowImage(WN_VIEW, img_in);
				
				if(flag==FALSE)
					printf(" FAILED:%s\n", truth[i].name + strlen(path_feret_img));
				
				int key;
				if(flag==FALSE && display>=2)
					key = cvWaitKey(0);
				else if(flag==FALSE && display>0)
					key = cvWaitKey(0);
				else
					key = cvWaitKey(10);
				if (key == VK_SPACE)
					cvWaitKey(0);
			}

			if (p) cFR.DeleteS_PEAK(p);
			cvReleaseImage(&img_in);
			img_in = NULL;
		}

		unsigned long time_delta = timeGetTime() - time_start;
		printf("\nTime:%d[ms]\n%d%% %d[ms]", time_delta, cnt_success*100/valid_files, time_delta/valid_files);

		//�I������
		delete [] truth;
		for (int i=0;i<files;i++)
			delete [] str_arr_filenames[i];
		delete [] str_arr_filenames;
		cvReleaseMemStorage (&storage);

		cvWaitKey (0);
	} else if (mode == 4 || mode == 14) {		//caltech TEST

		char str_tmp[1024];

		//cv
		const char *cascade_name = CASCADE_PATH;
		CvHaarClassifierCascade *cascade = 0;
		CvMemStorage *storage = 0;
		CvSeq *seq_faces;
		storage = cvCreateMemStorage (0);
		cascade = (CvHaarClassifierCascade *) cvLoad (cascade_name, 0, 0, 0);
		if (cascade==NULL)
			return 0;

		//�����f�[�^�̓ǂݍ���
		int files=450;
		CAFILES *truth = new CAFILES[files];
		ZeroMemory(truth, sizeof(CAFILES)*files);
		FILE* fp=NULL;
		fopen_s(&fp, path_caltech_tru, "r");
		if (fp==NULL) {
			printf("OPEN ERROR : %s\n", path_caltech_tru);
			delete [] truth;
			getchar();
			return 0;
		}
		double d;
		for (int i = 0; i < files; i++) {
			fscanf_s(fp, "%lf", &d);
			truth[i].lb.x = cvRound(d); 
		}
		for (int i = 0; i < files; i++) {
			fscanf_s(fp, "%lf", &d);
			truth[i].lb.y = cvRound(d); 
		}
		for (int i = 0; i < files; i++) {
			fscanf_s(fp, "%lf", &d);
			truth[i].lt.x = cvRound(d); 
		}
		for (int i = 0; i < files; i++) {
			fscanf_s(fp, "%lf", &d);
			truth[i].lt.y = cvRound(d); 
		}
		for (int i = 0; i < files; i++) {
			fscanf_s(fp, "%lf", &d);
			truth[i].rt.x = cvRound(d); 
		}
		for (int i = 0; i < files; i++) {
			fscanf_s(fp, "%lf", &d);
			truth[i].rt.y = cvRound(d); 
		}
		for (int i = 0; i < files; i++) {
			fscanf_s(fp, "%lf", &d);
			truth[i].rb.x = cvRound(d); 
		}
		for (int i = 0; i < files; i++) {
			fscanf_s(fp, "%lf", &d);
			truth[i].rb.y = cvRound(d); 
		}
		fclose(fp);
		for (int i = 0; i < files; i++) {
			truth[i].area.x = (truth[i].lt.x + truth[i].lb.x) / 2;
			truth[i].area.y = (truth[i].lt.y + truth[i].rt.y) / 2;
			truth[i].area.width = (truth[i].rt.x + truth[i].rb.x) / 2 - truth[i].area.x;
			truth[i].area.height = (truth[i].lb.y + truth[i].rb.y) / 2 - truth[i].area.y;
			//�̈��1/4�ɂ���D���̗̈��S�Ċ܂ޏꍇ�͐����Ƃ���
			truth[i].area.x += truth[i].area.width*3/8;
			truth[i].area.y += truth[i].area.height*7/16;
			truth[i].area.width /= 4;
			truth[i].area.height /= 8;			
		}
		//��������猟�o
		cvNamedWindow (WN_VIEW, 1);

		unsigned long time_start = timeGetTime();
		IplImage *img_tmp=NULL, *img_in=NULL;

		printf("success/progress/images\n");

		int cnt_success=0;
		int valid_files=0;
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);

		for (int i = 0; i < files; i++) {

			sprintf_s(str_tmp, 1024, "%s\\image_%04d.jpg", path_caltech_img, i+1);
			img_in = cvLoadImage(str_tmp, 0);
			if (img_in==NULL)	continue;

			valid_files++;

			S_PEAK *p=NULL;

			//-�猟�o
			int ret=0;
			int flag=FALSE;
			CvRect r;
			if (mode == 4) {

				ret = cFR.FaceDetection(img_in, &p, ((display<=1)?0:display));

				if (ret>0) {
					r = cFR.m_r_result;

					if (r.x < truth[i].area.x && r.x+r.width > truth[i].area.x+truth[i].area.width
						&& r.y < truth[i].area.y && r.y+r.height > truth[i].area.y+truth[i].area.height )  {
						cnt_success ++;
						flag=TRUE;
					}
				}

			} else {
				cvEqualizeHist (img_in, img_in);
				cvClearMemStorage (storage);
				seq_faces = cvHaarDetectObjects (img_in, cascade, storage, 1.11, 4, 0, cvSize (60, 60));				
				for (int j = 0; j < (seq_faces ? seq_faces->total : 0); j++) {
					CvRect r = *((CvRect *) cvGetSeqElem (seq_faces, j));
					if (r.x < truth[i].area.x && r.x+r.width > truth[i].area.x+truth[i].area.width
						&& r.y < truth[i].area.y && r.y+r.height > truth[i].area.y+truth[i].area.height ) {
						if (display>=1) {
							cvDrawRect(img_in, cvPoint(r.x, r.y), cvPoint(r.x + r.width, r.y + r.height), CV_RGB(255,255,255));
						}
						flag=TRUE;
						cnt_success ++;
						break;
					}
				}
			}

			printf("\r%4d/%4d/%4d %3d%%", cnt_success, valid_files, files, cnt_success*100/(valid_files));

			if (display>=1) {
				cvDrawRect(img_in, cvPoint(truth[i].area.x, truth[i].area.y), cvPoint(truth[i].area.x + truth[i].area.width, truth[i].area.y + truth[i].area.height), CV_RGB(0,0,0), 2);
				
				//if (flag)
				if ((flag || ret>0) && mode != 14) {
					S_PEAK *pp = p;
					while (pp) {
						cvDrawRect(img_in,
							pp->pos - cvPoint(pp->scale/2, pp->scale/2),
							pp->pos + cvPoint(pp->scale/2, pp->scale/2),
							CV_RGB(0,0,0));
						sprintf_s(str_tmp, 1024, "%.3f", pp->value);
						cvPutText(img_in, str_tmp, pp->pos + cvPoint(-pp->scale/2, pp->scale/2), &font, CV_RGB(255,255,255));
						pp = pp->next;
					}
					cvDrawRect(img_in, cvPoint(r.x, r.y), cvPoint(r.x + r.width, r.y + r.height), CV_RGB(255,255,255), 2);
				}

				cvShowImage(WN_VIEW, img_in);
				
				if(flag==FALSE)
					printf(" FAILED:%d\n", i+1);
				
				int key;
				if(flag==FALSE && display>=2)
					key = cvWaitKey(0);
				else if(flag==FALSE && display>0)
					key = cvWaitKey(0);
				else
					key = cvWaitKey(10);
				if (key == VK_SPACE)
					cvWaitKey(0);
			}

			if (p) cFR.DeleteS_PEAK(p);
			cvReleaseImage(&img_in);
			img_in = NULL;
		} 

		unsigned long time_delta = timeGetTime() - time_start;
		printf("\nTime:%d[ms]\n%d%% %d[ms]", time_delta, cnt_success*100/valid_files, time_delta/files);

		//�I������
		delete [] truth;
		cvReleaseMemStorage (&storage);

		cvWaitKey (0);

	} else if (mode == 5 || mode == 15) {		//barca TEST

		char str_tmp[1024];

		//cv
		const char *cascade_name = CASCADE_PATH;
		CvHaarClassifierCascade *cascade = 0;
		CvMemStorage *storage = 0;
		CvSeq *seq_faces;
		storage = cvCreateMemStorage (0);
		cascade = (CvHaarClassifierCascade *) cvLoad (cascade_name, 0, 0, 0);
		if (cascade==NULL)
			return 0;

		//�����f�[�^�̓ǂݍ���
		//-�����f�[�^�̍\�����悭�킩��Ȃ��̂Œ��S�t�߂𐳉��ɂ��Ă���
		CvRect Truth = cvRect(64-16, 64, 32, 12);

		//��摜�t�@�C���̌���
		char **str_arr_filenames=NULL;
		int files = SearchFolder(path_barca_img, "*.png", &str_arr_filenames);
		printf("%d files\n", files);

		//��������猟�o
		cvNamedWindow (WN_VIEW, 1);

		unsigned long time_start = timeGetTime();
		IplImage *img_tmp=NULL, *img_in=NULL;

		printf("success/progress/images\n");

		int cnt_success=0;
		int valid_files=0;
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);

		for (int i = 0; i < files; i++) {

			//sprintf_s(str_tmp, 1024, "%s\\%s", path_barca_img, str_arr_filenames[i]);
			img_in = cvLoadImage(str_arr_filenames[i], 0);
			if (img_in==NULL)	continue;

			valid_files++;

			S_PEAK *p=NULL;

			//-�猟�o
			int ret=0;
			int flag=FALSE;
			CvRect r;
			if (mode == 5) {
 				ret = cFR.FaceDetection(img_in, &p, ((display<=1)?0:display));

				if (ret>0) {
					r = cFR.m_r_result;

					if (r.x < Truth.x && r.x+r.width > Truth.x+Truth.width
						&& Truth.y && r.y+r.height > Truth.y+Truth.height )  {
						cnt_success ++;
						flag=TRUE;
					}
				}

			} else {
				cvEqualizeHist (img_in, img_in);
				cvClearMemStorage (storage);
				seq_faces = cvHaarDetectObjects (img_in, cascade, storage, 1.11, 4, 0, cvSize (60, 60));				
				for (int j = 0; j < (seq_faces ? seq_faces->total : 0); j++) {
					CvRect r = *((CvRect *) cvGetSeqElem (seq_faces, j));
					if (r.x < Truth.x && r.x+r.width > Truth.x+Truth.width
						&& r.y < Truth.y && r.y+r.height > Truth.y+Truth.height ) {
						if (display>=1) {
							cvDrawRect(img_in, cvPoint(r.x, r.y), cvPoint(r.x + r.width, r.y + r.height), CV_RGB(255,255,255));
						}
						flag=TRUE;
						cnt_success ++;
						break;
					}
				}
			}

			printf("\r%4d/%4d/%4d %3d%%", cnt_success, valid_files, files, cnt_success*100/(valid_files));

			if (display>=1) {
				cvDrawRect(img_in, cvPoint(Truth.x, Truth.y), cvPoint(Truth.x + Truth.width, Truth.y + Truth.height), CV_RGB(0,0,0), 2);
				
				//if (flag)
				if ((flag || ret>0) && mode == 5) {
					S_PEAK *pp = p;
					while (pp) {
						cvDrawRect(img_in,
							pp->pos - cvPoint(pp->scale/2, pp->scale/2),
							pp->pos + cvPoint(pp->scale/2, pp->scale/2),
							CV_RGB(0,0,0));
						sprintf_s(str_tmp, 1024, "%.3f", pp->value);
						cvPutText(img_in, str_tmp, pp->pos + cvPoint(-pp->scale/2, pp->scale/2), &font, CV_RGB(255,255,255));
						pp = pp->next;
					}
					cvDrawRect(img_in, cvPoint(r.x, r.y), cvPoint(r.x + r.width, r.y + r.height), CV_RGB(255,255,255), 2);
				}

				cvShowImage(WN_VIEW, img_in);
				
				if(flag==FALSE)
					printf(" FAILED:%d\n", i+1);
				
				int key;
				if(flag==FALSE && display>=2)
					key = cvWaitKey(0);
				else if(flag==FALSE && display>0)
					key = cvWaitKey(0);
				else
					key = cvWaitKey(10);
				if (key == VK_SPACE)
					cvWaitKey(0);
			}

			if (p) cFR.DeleteS_PEAK(p);
			cvReleaseImage(&img_in);
			img_in = NULL;
		} 

		unsigned long time_delta = timeGetTime() - time_start;
		printf("\nTime:%d[ms]\n%d%% %d[ms]", time_delta, cnt_success*100/valid_files, time_delta/files);

		//�I������
		for (int i=0;i<files;i++)
			delete [] str_arr_filenames[i];
		delete [] str_arr_filenames;
		cvReleaseMemStorage (&storage);

		cvWaitKey (0);
	}

er:

	// �E�B���h�E���
	cvDestroyAllWindows();

	// ���f���摜���
	if (mode < 10) {
		if (file_mo != file_mo_default) {
			for(int i=0; i<models; i++) {
				delete [] file_mo[i];
			}

			delete [] file_mo;
		}
		if (modeltype != modeltype_default) {
			delete [] modeltype;
		}
	}

	return 0;
}

// �L���摜�t�H���_���J���ăt�@�C�����X�g����� ->
int SearchFolder(char *folder, char *filter, char ***path)
{
	wchar_t wc_folder[MAX_PATH];
	wchar_t wc_filter[MAX_PATH];

	wchar_t finder[MAX_PATH];
	wchar_t tstrtmp[MAX_PATH];
	
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wc_folder, MAX_PATH, folder, _TRUNCATE);
	mbstowcs_s(&convertedChars, wc_filter, MAX_PATH, filter, _TRUNCATE);

	swprintf_s(finder, MAX_PATH, L"%s\\%s", wc_folder, wc_filter);

	int faces = 0;
	{
		_wfinddata64i32_t c_file;

		intptr_t hFile = _wfindfirst(finder, &c_file);

		if (hFile == -1L) {
			return 0;
		}
		faces ++;
		while( _wfindnext( hFile, &c_file ) == 0 ){
			faces ++;
		}
	}
	if (faces==0) return 0;
	int i;
	{
		//�t�@�C�����̋L���̈�����
		*path = new char*[faces];
		for (i = 0; i < faces; i ++) {
			(*path)[i] = new char[MAX_PATH];
		}

		//������񌟍����ăt�@�C�������i�[
		_wfinddata64i32_t c_file;

		i = 0;
		intptr_t hFile = _wfindfirst(finder, &c_file);
		swprintf_s(tstrtmp, MAX_PATH, L"%s\\%s", wc_folder, c_file.name);//�t�H���_���ƃt�@�C�������Ȃ���
		wcstombs_s(NULL, (*path)[i], MAX_PATH, tstrtmp, MAX_PATH);//wchar_t����char�ɕϊ�

		i = 1;
		while( _wfindnext( hFile, &c_file ) == 0 ) {
			swprintf_s(tstrtmp, MAX_PATH, L"%s\\%s", wc_folder, c_file.name);//�t�H���_���ƃt�@�C�������Ȃ���
			wcstombs_s(NULL, (*path)[i], MAX_PATH, tstrtmp, MAX_PATH);//wchar_t����char�ɕϊ�
			i ++;
		}
	}
	//_findclose( hFile );

	return i;
}

void ReleaseSearchFolder(char ***path, int faces)
{
	for (int i = 0; i < faces; i ++) {
		delete [] (*path)[i];
	}
	delete [] (*path);
	*path=NULL;
}

//�t�@�C�����J���_�C�A���O
//http://www.kumei.ne.jp/c_lang/sdk2/sdk_104.htm
int ofd(char *fn, char *filter, char *title) {
	cvNamedWindow(WN_VIEW, 1);
	HWND hwnd = FindWindow("Main HighGUI class", WN_VIEW);

	OPENFILENAME ofn;
	//char szFileName[MAX_PATH];
    char szFile[MAX_PATH];

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ZeroMemory(szFile, sizeof(char)*MAX_PATH);
	ZeroMemory(fn, sizeof(char)*MAX_PATH);
	//ZeroMemory(szFileName, sizeof(char)*MAX_PATH);
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;//"All files(*.*)\0*.*\0\0";
    ofn.lpstrFile = fn;
    ofn.lpstrFileTitle = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "*";
    ofn.lpstrTitle = title;
    if(GetOpenFileName(&ofn) == 0)
        return FALSE;
	return TRUE;
}
