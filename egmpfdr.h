// �猟�o�pEGM�N���X
class C_EGM_FDR : public C_EGM
{
public:
	C_EGM_FDR();
	~C_EGM_FDR();
	int EGM_FDR(int times, int range, int reso=1);
	int EGM_FDR_Main(int range);
	int EGM_FDR_SR_Main(int range, int reso=1);
	int SweepFinding(int display=0, IplImage **img_ret=NULL, int pitch=1);
	int SetNodeWeight(int type=0);
//	int SetWeightDiff(EGM_VALUETYPE w){m_weight_diff=w;return TRUE;};
	EGM_VALUETYPE GetDistortionValue();
protected:
	float GetJet_st(S_POINT pos, int dim);
	float GetJet_dy(S_POINT pos, int dim);
	int *m_v_log;//�U���h�~:�ߋ�5��̃G�l���M���L�����Ă���
	int m_v_log_cnt;
	//EGM_VALUETYPE m_weight_diff;//���Z���ɂ�����W��
};

struct S_PEAK {
	CvPoint pos;
	int scale;
	float value;
	S_PEAK *next;
	C_GRID *grid;
};

// �猟�o�N���X
class C_FACE_DETECT_RECOG
{
public:
	C_FACE_DETECT_RECOG();
	~C_FACE_DETECT_RECOG();

	int PreProcess(char **file_model);
	int FaceDetection(IplImage *imgin, S_PEAK **ret, int display=FALSE, int store_enMap=FALSE);
	void DeleteS_PEAK(S_PEAK *peak);
	float FaceRecognition(IplImage *imgin, int wi, C_GRID **grid_r=NULL, int display=FALSE);

	IplImage *GetEnergyMap();
	IplImage *GetResultImage();

	int m_gabor_ori;	//�K�{�[���p�x����
	
	int m_layers_in;	//���͉摜���C���[���i���ʂ�1�j
	int m_layers_mo;	//���f���摜���C���[��
	int *m_width_in;	//���͉摜���C���[�𑜓x���X�g
	int *m_width_mo;	//�L���摜���C���[�𑜓x���X�g

	int m_node_weight;	//�m�[�h�ɏd�݂����邩�ǂ���

	//int m_fastdetect;	//���͑��̌��o�𑜓x���Œ肷�邱�ƂŌ��o��������
	//int m_ffd_reso;		//�������o���̓��͉摜�𑜓xid
	//int m_detect_depth;	//�猟�o�X�P�[����d (1/(2^d)�̉𑜓x�܂Ō��o)
	//int m_find_reso;
	//int m_recog_reso;	//�L���摜�̍ō��𑜓xid

	int m_filter_noise; //�����Jet�̏������s�����ǂ���
	float m_th_weak_feature;//����Jet�����������l

	double	m_Nu;
	double	m_Sigma;
	double	m_F;
	S_RECT m_grid_m;
	S_RECT m_grid_i;
	int m_grid_res;
	int *m_egm_times;
	int *m_egm_range;
	EGM_VALUETYPE *m_weight_dist;
	//EGM_VALUETYPE m_weight_diff;//���Z���ɂ�����W��

	int m_models;
	int *m_modeltype;//���f�����������f��0���s�������f��1��
	//IplImage **m_img_in;
	IplImage **m_img_mo;
	//IplImage **m_jet_in;
	IplImage **m_jet_mo;
	IplImage *m_jet_in_disp;
	IplImage **m_enmap;
	IplImage *m_img_result;

	CvGabor* m_gabor;
	CvRect m_r_result;
	EGM_VALUETYPE m_v_result;
	C_GRID m_r_grid;
	char m_str_view_window_name[MAX_PATH];

	int m_show_filter;
	int m_stage;
	int m_sync_scale;//���͉摜�ƋL���摜�𓯂��X�P�[���̉摜�Ƃ��Ĉ������ǂ���
	int m_sweepfinding;//�S�摖���猟�o
	int m_superreso;//���𑜓x�v�Z
	int m_show_extended;//�摜�̊g��\��

protected:
	int PeakDetection(IplImage *img, int mw, float th_v, float th_ov, S_PEAK **p_listtop, S_PEAK **p_listlast);
	int PeakElimination(S_PEAK **p_listtop, float th_ov);
	int m_egmcnt;
};
