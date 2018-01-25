// 以下のヘッダが先にインクルードされている必要がある
//#include <opencv\cv.h>

#ifndef CVGABOR_H
#include "_cvgabor.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) {if(x){x->Release();delete x;x=NULL;}}
#endif

// 各種設定
#define EGM_MAXLINKS 8 //リンクの最大数
typedef float EGM_VALUETYPE; //計算に使う型
#define EGM_SQRT(x) cvSqrt(x) //sqrt関数の選択

struct S_POINT
{
	EGM_VALUETYPE x;
	EGM_VALUETYPE y;
};

struct S_RECT
{
	EGM_VALUETYPE x;
	EGM_VALUETYPE y;
	EGM_VALUETYPE width;
	EGM_VALUETYPE height;
};

struct SDLNODE
{
	BYTE active; //ノードの有効/無効 無効のときは0
	S_POINT pos_st; //固定側のノード座標
	S_POINT pos_dy; //移動側のノード座標
	CvPoint pos_sti; //固定側のノード座標
	CvPoint pos_dyi; //移動側のノード座標
	EGM_VALUETYPE weight; //重み11/08/16追加
	EGM_VALUETYPE value; //最後のマッチング評価値
	int stopcount;			//ノード連続停止フレーム数
	int link[EGM_MAXLINKS]; //リンク先のインデックス, -1のときはリンクなし
	BYTE lock; //0以外ならノードを移動しないようにする
};

class C_GRID
{
public:
	C_GRID();
	~C_GRID();

	// 格子を格納する領域を生成
	int Create(int nodes);

	// reso_x×reso_yの目の正方格子を作る(ノードの数は+1個ずつになる)
	int CreateSquareGrid(S_RECT *rArea, int reso_x, int reso_y, S_RECT *rDynamicArea=NULL);
	int CreateInnerCircleSquareGrid(S_RECT *rArea, int reso_x, int reso_y, S_RECT *rDynamicArea=NULL);
	int CreateRoundCornerSquareGrid(S_RECT *rArea, int reso_x, int reso_y, S_RECT *rDynamicArea);

	// 格子を描く
	int DrawGrid(IplImage *, int dy, CvScalar nodecolor, CvScalar linkcolor, CvScalar lockedcolor, int showvalue=FALSE);
	int DrawGridB(IplImage *, int dy, CvScalar nodecolor, CvScalar linkcolor, CvScalar lockedcolor, CvScalar motioncolor);

	int GetArea(S_RECT *r);

	void DebugTrace();//構造体の状態をテキスト出力

	// データコピー
	int Copy(C_GRID* from);

	// セーフティデータアクセス
	int IsInited();
	SDLNODE *GetBuffer();
	int GetBuffer(int index, SDLNODE **ppNode);
	int SetBuffer(int index, SDLNODE *ppNode);
	int GetLinks(int index);
	int GetNodeBufferLength();
	int GetActiveNodeCount();

	// ノード操作
	int DeleteNode(int index);
	int DeleteLink(int index, int linkindex);
	int AddNode(S_POINT p);
	int AddLink(int iFrom, int iTo);
	int MergeNode(int i1, int i2);

	int Render(int w_st, int h_st, int w_dy, int h_dy);//floatをintへ変換
	int UnRender(int w_st, int h_st, int w_dy, int h_dy);//intをfloatへ変換
	int ResetValue();

	void Release();

protected:
	int m_nodes;
	SDLNODE *m_node;
};

class C_EGM
{
public:
	C_EGM();
	~C_EGM();
	void Release();

	// EGM実行(最大回数, 計算をやめる評価値)
	// 戻り値は実際に実行された回数
	int EGM(int times, int range=1);

	void Reset(); //マッチングの状態をリセット

	// データアクセス
	EGM_VALUETYPE GetValue() {return m_value;} //評価値を取得

	int SetParam(EGM_VALUETYPE weight_sim, EGM_VALUETYPE weight_dist);
	int SetGrid(C_GRID*);
	int SetJet_st(IplImage**, int images, int dy=0);
	int SetJet_dy(IplImage** img, int images) {return SetJet_st(img, images, 1);}
	int SetJet_st_b(IplImage*, int images, int dy=0);
	int SetJet_dy_b(IplImage* img, int images) {return SetJet_st_b(img, images, 1);}

	int GetGrid(C_GRID*);
	// 格子を描く
	int DrawGrid(IplImage *, int dy, CvScalar nodecolor, CvScalar linkcolor, CvScalar lockedcolor);

	// 格子の歪み計算
	double GetDistortion();

	// パブリック変数(外部から書き換えられても支障がない)
	int m_lastmovednodes; //最後の更新で動いたノードの数

	// ノードを固定するしきい値を指定
	int SetThLock(EGM_VALUETYPE th){m_th_lock = th; return TRUE;};

	void SetSuperReso(int enabled){m_superreso=enabled;};

protected:
	int EGMMain(int range=1); //1回計算を実行 引数は探索範囲

	int m_superreso;//超解像度モード
	int m_width_st, m_height_st;
	int m_width_dy, m_height_dy;
	int m_jetdim; // 1ピクセルあたりのJetの数
	EGM_VALUETYPE *m_jet_st; // Jet, m_width * m_height * m_jetcount個
	EGM_VALUETYPE *m_jet_dy;
	C_GRID *m_grid; // 格子
	C_GRID *m_grid_backup; // 格子の初期状態(Resetで使う)

	EGM_VALUETYPE m_th_lock; //固定側jetのノルムがこの数値以下ならノードを固定する

	int m_LoopCount; //計算回数
	EGM_VALUETYPE m_weight_sim, m_weight_dist; //係数
	EGM_VALUETYPE m_value; //評価値(エネルギ)
};

void cvt_8U1_to_32F1(IplImage *src, IplImage *dst);
void cvt_8U1_to_8U3(IplImage *src, IplImage *dst);
void cvt_32F1_to_8U3(IplImage *src, IplImage *dst);
CvPoint cvpPlus(CvPoint& a, CvPoint &b);

inline S_POINT s_point( EGM_VALUETYPE x, EGM_VALUETYPE y )
{
	S_POINT p;
	p.x = x;
	p.y = y;
	return p;
}

inline S_POINT s_point_normal( EGM_VALUETYPE x, EGM_VALUETYPE y, EGM_VALUETYPE w, EGM_VALUETYPE h )
{
	S_POINT p;
	if (w!=0)
		p.x = x/w;
	else
		p.x = 0;
	if (h!=0)
		p.y = y/h;
	else
		p.y = 0;
	return p;
}

inline S_RECT s_rect_2p( EGM_VALUETYPE x1, EGM_VALUETYPE y1, EGM_VALUETYPE x2, EGM_VALUETYPE y2 )
{
    S_RECT r;

    r.x = x1;
    r.y = y1;
    r.width = x2 - x1;
    r.height = y2 - y1;

    return r;
}

inline S_RECT s_rect( EGM_VALUETYPE x, EGM_VALUETYPE y, EGM_VALUETYPE width, EGM_VALUETYPE height )
{
    S_RECT r;

    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;

    return r;
}

inline S_RECT operator *(S_RECT a, EGM_VALUETYPE v)
{
    S_RECT r;

    r.x = a.x * v;
    r.y = a.y * v;
	r.width = a.width * v;
    r.height = a.height * v;

    return r;
}

inline S_POINT operator +(S_POINT a, S_POINT b) {
	S_POINT r;
	r.x = a.x + b.x;
	r.y = a.y + b.y;
	return r;
}

inline S_POINT operator -(S_POINT a, S_POINT b) {
	S_POINT r;
	r.x = a.x - b.x;
	r.y = a.y - b.y;
	return r;
}

inline CvPoint operator +(CvPoint a, CvPoint b) {
	CvPoint r;
	r.x = a.x + b.x;
	r.y = a.y + b.y;
	return r;
}

inline CvPoint operator -(CvPoint a, CvPoint b) {
	CvPoint r;
	r.x = a.x - b.x;
	r.y = a.y - b.y;
	return r;
}

//dot product
inline EGM_VALUETYPE operator *(S_POINT a, S_POINT b) {
	EGM_VALUETYPE na = a.x*a.x+a.y*a.y;
	EGM_VALUETYPE nb = b.x*b.x+b.y*b.y;
	if (na > 0 && nb > 0)
		return (a.x * b.x + a.y * b.y) / (EGM_SQRT(na) * EGM_SQRT(nb));
	return 0;
}

//dot product
inline float operator *(CvPoint a, CvPoint b) {
	int na = a.x*a.x+a.y*a.y;
	int nb = b.x*b.x+b.y*b.y;
	if (na > 0 && nb > 0)
		return (float)(a.x * b.x + a.y * b.y) / (cvSqrt((float)na) * cvSqrt((float)nb));
	return 0;
}

//norm
inline EGM_VALUETYPE s_size(S_POINT a) {
	EGM_VALUETYPE r = a.x * a.x + a.y * a.y;
	if (r > 0)
		return (EGM_SQRT(r));
	return 0;
}

//norm
inline float cvp_size(CvPoint a) {
	int r = a.x * a.x + a.y * a.y;
	if (r > 0)
		return (cvSqrt((float)r));
	return 0;
}

inline CvPoint S_POINTtoCvPoint(S_POINT p, int w, int h) {
	return cvPoint((int)(p.x * w), (int)(p.y * h));
//	return cvPoint(cvRound(p.x * w), cvRound(p.y * h));
}

inline CvPoint S_POINTtoCvPoint(S_POINT p, IplImage *pImg) {
	return cvPoint((int)(p.x * pImg->width), (int)(p.y * pImg->height));
//	return cvPoint(cvRound(p.x * pImg->width), cvRound(p.y * pImg->height));
}

inline S_POINT CvPointtoS_POINT(CvPoint p, IplImage *pImg) {
	return s_point((EGM_VALUETYPE)p.x / pImg->width, (EGM_VALUETYPE)p.y / pImg->height);
}

inline S_POINT CvPointtoS_POINT(CvPoint p, int w, int h) {
	return s_point((EGM_VALUETYPE)p.x / w, (EGM_VALUETYPE)p.y / h);
}

inline CvRect S_RECTtoCvRect(S_RECT p, int w, int h) {
	return cvRect((int)(p.x * w), (int)(p.y * h), (int)(p.width * w), (int)(p.height * h));
}

inline CvRect operator +(CvRect a, CvRect b) {
	return cvRect(a.x + b.x, a.y + b.y, a.width + b.width, a.height + b.height);
}

inline S_RECT operator +(S_RECT a, S_RECT b) {
	return s_rect(a.x + b.x, a.y + b.y, a.width + b.width, a.height + b.height);
}

void srExtendArea(S_RECT *r, float per, int correct_over=TRUE);

namespace N_EGM{
	CvScalar GetDisplayColor(int lv);
}