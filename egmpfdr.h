// 顔検出用EGMクラス
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
	int *m_v_log;//振動防止:過去5回のエネルギを記憶しておく
	int m_v_log_cnt;
	//EGM_VALUETYPE m_weight_diff;//減算項にかける係数
};

struct S_PEAK {
	CvPoint pos;
	int scale;
	float value;
	S_PEAK *next;
	C_GRID *grid;
};

// 顔検出クラス
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

	int m_gabor_ori;	//ガボール角度分解数
	
	int m_layers_in;	//入力画像レイヤー数（当面は1）
	int m_layers_mo;	//モデル画像レイヤー数
	int *m_width_in;	//入力画像レイヤー解像度リスト
	int *m_width_mo;	//記憶画像レイヤー解像度リスト

	int m_node_weight;	//ノードに重みをつけるかどうか

	//int m_fastdetect;	//入力側の検出解像度を固定することで検出を高速化
	//int m_ffd_reso;		//高速検出時の入力画像解像度id
	//int m_detect_depth;	//顔検出スケール数d (1/(2^d)の解像度まで検出)
	//int m_find_reso;
	//int m_recog_reso;	//記憶画像の最高解像度id

	int m_filter_noise; //微弱なJetの除去を行うかどうか
	float m_th_weak_feature;//微弱Jet除去しきい値

	double	m_Nu;
	double	m_Sigma;
	double	m_F;
	S_RECT m_grid_m;
	S_RECT m_grid_i;
	int m_grid_res;
	int *m_egm_times;
	int *m_egm_range;
	EGM_VALUETYPE *m_weight_dist;
	//EGM_VALUETYPE m_weight_diff;//減算項にかける係数

	int m_models;
	int *m_modeltype;//モデルが正解モデル0か不正解モデル1か
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
	int m_sync_scale;//入力画像と記憶画像を同じスケールの画像として扱うかどうか
	int m_sweepfinding;//全域走査顔検出
	int m_superreso;//超解像度計算
	int m_show_extended;//画像の拡大表示

protected:
	int PeakDetection(IplImage *img, int mw, float th_v, float th_ov, S_PEAK **p_listtop, S_PEAK **p_listlast);
	int PeakElimination(S_PEAK **p_listtop, float th_ov);
	int m_egmcnt;
};
