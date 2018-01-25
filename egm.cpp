//#include <cv.h>
//#include <highgui.h>

#include "stdafx.h"
#include "_cvgabor.h"
#include "egm.h"

// �Ȃ���CvPoint��+�I�y���[�^���Ȃ��̂ő�p
inline CvPoint cvpPlus(CvPoint& a, CvPoint &b){
	return cvPoint(a.x+b.x, a.y+b.y);
}

C_GRID::C_GRID()
{
	m_nodes = 0;
	m_node = NULL;
}

C_GRID::~C_GRID()
{
	Release();
}

void C_GRID::Release()
{
	if ( m_node ) {
		delete [] m_node;
		m_node = NULL;
	}
	m_nodes = 0;
}

int C_GRID::Render(int w_st, int h_st, int w_dy, int h_dy)
{
	for (int i = 0; i < m_nodes; i ++) {
		m_node[i].pos_sti.x = (int)(m_node[i].pos_st.x * w_st);
		m_node[i].pos_sti.y = (int)(m_node[i].pos_st.y * h_st);
		m_node[i].pos_dyi.x = (int)(m_node[i].pos_dy.x * w_dy);
		m_node[i].pos_dyi.y = (int)(m_node[i].pos_dy.y * h_dy);
	}

	return TRUE;
}

int C_GRID::UnRender(int w_st, int h_st, int w_dy, int h_dy)
{

	for (int i = 0; i < m_nodes; i ++) {
		m_node[i].pos_st.x = (float)m_node[i].pos_sti.x / w_st;
		m_node[i].pos_st.y = (float)m_node[i].pos_sti.y / h_st;
		m_node[i].pos_dy.x = (float)m_node[i].pos_dyi.x / w_dy;
		m_node[i].pos_dy.y = (float)m_node[i].pos_dyi.y / h_dy;
	}

	return TRUE;
}

int C_GRID::IsInited()
{
	if ( m_node != NULL && m_nodes > 0 )
		return TRUE;
	else
		return FALSE;
}

int C_GRID::Copy(C_GRID* from)
{
	int nodes = from->GetNodeBufferLength();

	if ( nodes <= 0 ) return FALSE;

	Release();

	m_nodes = nodes;
	m_node = new SDLNODE [m_nodes];

	memcpy(m_node, from->GetBuffer(), sizeof(SDLNODE) * m_nodes );

	return TRUE;
}

// �o�b�t�@�̃|�C���^���擾
// �f�[�^��j�󂷂�\��������̂Œ���
SDLNODE *C_GRID::GetBuffer()
{
	return m_node;
}

int C_GRID::GetBuffer(int index, SDLNODE **ppNode)
{
	memcpy(*ppNode, m_node + index, sizeof(SDLNODE));

	return TRUE;
}

int C_GRID::SetBuffer(int index, SDLNODE *ppNode)
{
	memcpy(m_node + index, ppNode,  sizeof(SDLNODE));

	return TRUE;
}

// �m�[�h�o�b�t�@����Ԃ�
int C_GRID::GetNodeBufferLength()
{
	return m_nodes;
}

// �A�N�e�B�u�ȃm�[�h�̐���Ԃ�
int C_GRID::GetActiveNodeCount()
{
	int i, c = 0;

	for (i = 0; i < m_nodes; i ++)
		if (m_node[i].active)
			c ++;

	return c;
}

// �m�[�h�̒ǉ�
int C_GRID::AddNode(S_POINT p)
{

	// active := 0 �ɂȂ��Ă���m�[�h��T��
	int idx = 0;
	while ( idx < m_nodes ) {
		if ( ! m_node[idx].active )
			break;
		idx ++;
	}

	if ( idx == m_nodes ) {
		// �z�񂪈�t�Ȃ�Η̈��ǉ�����
		// �m�[�h�P���傫�ȗ̈�������
		// �����Ɍ��݂�m_node���R�s�[
		SDLNODE *dataold = m_node;
		m_node = new SDLNODE[m_nodes + 1];
		memcpy_s(m_node, sizeof(SDLNODE) * (m_nodes+1), dataold,  sizeof(SDLNODE) * m_nodes);
		// �Â��̈���폜
		delete [] dataold;
	}

	// �m�[�h�̏�Ԃ������ݒ�
	memset(m_node + idx, 0, sizeof(SDLNODE));
	m_node[idx].active = 1;
	m_node[idx].weight = 1;
	m_node[idx].pos_st.x = p.x;
	m_node[idx].pos_st.y = p.y;
	m_node[idx].pos_dy.x = p.x;
	m_node[idx].pos_dy.y = p.y;
	int i;
	for (i = 0; i < EGM_MAXLINKS; i ++)
		m_node[idx].link[i] = -1;

	// �㏈��
	m_nodes ++;
	return idx;
}

// �m�[�h�̍폜
int C_GRID::DeleteNode(int index)
{
	// active�t���O��0��
	m_node[index].active = 0;

	// �����N������
	int i;
	for (i = 0; i < EGM_MAXLINKS; i ++) {
		if (m_node[index].link[i] < 0) continue;
		DeleteLink(index, i);
	}
	return TRUE;
}

int C_GRID::ResetValue()
{
	for (int i = 0; i < m_nodes; i ++) {
		m_node[i].value = -1000.0f;
	}
	return TRUE;
}

// �����N��ݒ�
int C_GRID::AddLink(int iFrom, int iTo)
{
	// From��: �����N�d���`�F�b�N
	int i;
	for (i = 0; i < EGM_MAXLINKS; i ++) {
		if ( m_node[iFrom].link[i] == iTo )
			return TRUE;
	}

	// From��: -1�ɂȂ��Ă��郊���N��T��
	i = 0;
	while ( 1 ) {
		if ( m_node[iFrom].link[i] < 0 )
			break;

		i ++;

		if (i >= EGM_MAXLINKS)
			return FALSE; //�����N�������ς�
	}

	// To��: -1�ɂȂ��Ă��郊���N��T��
	int j = 0;
	while ( 1 ) {
		//if ( m_node[iFrom].link[j] == iFrom )
		//	return TRUE; //���łɃ����N����Ă���

		if ( m_node[iFrom].link[j] < 0 )
			break;

		j ++;

		if (j >= EGM_MAXLINKS)
			return FALSE; //�����N�������ς�
	}

	// �����N��ݒ�
	m_node[iFrom].link[i] = iTo;
	m_node[iTo  ].link[j] = iFrom;

	return TRUE;
}

void C_GRID::DebugTrace()
{
#ifdef _DEBUG
	for (int i = 0; i < m_nodes; i ++) {
		printf("[%3d] a=%d dy=(%5.3f %5.3f) st=(%5.3f %5.3f) lnk=(",
			i, m_node[i].active, m_node[i].pos_dy.x, m_node[i].pos_dy.y, m_node[i].pos_st.x, m_node[i].pos_st.y);
		for (int j = 0; j < EGM_MAXLINKS; j ++) {
			printf("%d ", m_node[i].link[j]);
		}
		printf(")\n");
	}
#endif
}

// ���݂̃����N������
int C_GRID::DeleteLink(int index, int linkindex)
{
	int i;
	int iDst = m_node[index].link[linkindex];

	if ( iDst < 0 ) return FALSE;

	m_node[index].link[linkindex] = -1;

	// �����N��Ŏ����ւ̃����N��T���ĉ���
	for (i = 0; i < EGM_MAXLINKS; i ++) {
		if (m_node[iDst].link[i] == index) {
			m_node[iDst].link[i] = -1;
			return TRUE;
		}
	}

	return FALSE;
}

// �m�[�h�̌���
int C_GRID::MergeNode(int i1, int i2)
{
	return FALSE;
}

// �O�ڂ����`�̈��Ԃ�
int C_GRID::GetArea(S_RECT *r)
{
	if (m_nodes < 1) return FALSE;
	S_RECT sum;
	int cnt = 0;
	for (int i = 0; i < m_nodes; i ++) {
		if (m_node[i].active) {
			if (cnt>0) {
				if (m_node[i].pos_dy.x < sum.x)
					sum.x = m_node[i].pos_dy.x;
				if (m_node[i].pos_dy.y < sum.y)
					sum.y = m_node[i].pos_dy.y;
				if (m_node[i].pos_dy.x > sum.width)
					sum.width = m_node[i].pos_dy.x;
				if (m_node[i].pos_dy.y > sum.height)
					sum.height = m_node[i].pos_dy.y;
			} else {
				sum.x = m_node[i].pos_dy.x;
				sum.y = m_node[i].pos_dy.y;
				sum.width = m_node[i].pos_dy.x;
				sum.height = m_node[i].pos_dy.y;
			}

			cnt++;
		}
	}
	*r = s_rect_2p(sum.x, sum.y, sum.width, sum.height);
	return TRUE;
}
// �i�q���i�[����̈�𐶐�
int C_GRID::Create(int nodes)
{
	if (nodes < 1) return FALSE;

	Release();

	m_node = new SDLNODE [nodes];
	m_nodes = nodes;

	// �m�[�h�̏�Ԃ������ݒ�
	memset(m_node, 0, sizeof(SDLNODE) * nodes);

	int i, j;
	for (i = 0; i < nodes; i ++) {
		for (j = 0; j < EGM_MAXLINKS; j ++)
			m_node[i].link[j] = -1;
	}

	return TRUE;
}

// reso_x�~reso_y�̖ڂ̐����i�q�����(�m�[�h�̐���+1���ɂȂ�)
// rDynamicArea�͈ړ����̃m�[�h�����͈͂�ʌɎw��ł���DNULL�ɂ���ΌŒ葤�Ɠ����ʒu�ɂȂ�
int C_GRID::CreateSquareGrid(S_RECT *rArea, int reso_x, int reso_y, S_RECT *rDynamicArea)
{
	// �������m��
	if ( ! Create( (reso_x + 1) * (reso_y + 1) ) )
		return FALSE;

	EGM_VALUETYPE x0, y0, w, h;
	x0 = rArea->x;
	y0 = rArea->y;
	w = rArea->width;
	h = rArea->height;

	EGM_VALUETYPE x0d, y0d, wd, hd;
	if (rDynamicArea!=NULL) {
		x0d = rDynamicArea->x;
		y0d = rDynamicArea->y;
		wd = rDynamicArea->width;
		hd = rDynamicArea->height;
	} else {
		x0d = x0;
		y0d = y0;
		wd = w;
		hd = h;
	}
	int x, y, p = 0;

	// �s�b�`�����ɂ��邽�߁C��Ƀs�b�`�����߂�i�E/���[���W�͎w��ǂ���ɂȂ�Ȃ��Ȃ�j
	EGM_VALUETYPE pitch_x = w / reso_x;
	EGM_VALUETYPE pitch_y = h / reso_y;
	EGM_VALUETYPE pitch_xd = wd / reso_x;
	EGM_VALUETYPE pitch_yd = hd / reso_y;

	// �m�[�h���쐬
	for (y = 0; y < reso_y+1; y ++) {
		for (x = 0; x < reso_x+1; x ++) {
			m_node[p].pos_st = s_point(x0 + (pitch_x * x), y0 + (y * pitch_y));
			if ( rDynamicArea == NULL )
				m_node[p].pos_dy = m_node[p].pos_st;
			else
				m_node[p].pos_dy = s_point(x0d + (pitch_xd * x), y0d + (y * pitch_yd));
			m_node[p].active = TRUE;
			m_node[p].lock = FALSE;
			//if ( x == 0 || x == reso_x || y == 0 || y == reso_y)
			//	m_node[p].lock = TRUE;//�O���Œ�
			p ++;
		}
	}

	// �����N���쐬
	for (y = 0; y < reso_y+1; y ++) {
		for (x = 0; x < reso_x+1; x ++) {
			p = x + y * (reso_x+1);
			//// ���̃m�[�h��
			//if (x > 0) {
			//	AddLink(p, p - 1);
			//}
			// �E�̃m�[�h��
			if (x < reso_x) {
				AddLink(p, p + 1);
			}
			////// ��̃m�[�h��
			//if (y > 0) {
			//	AddLink(p, p - (reso_x + 1));
			//}
			// ���̃m�[�h��
			if (y < reso_y) {
				AddLink(p, p + (reso_x + 1));
			}
		}
	}

	// �K�v�ȏꍇ�́C�����Ń����N�����v�Z

	return TRUE;
}
int C_GRID::CreateInnerCircleSquareGrid(S_RECT *rArea, int reso_x, int reso_y, S_RECT *rDynamicArea)
{
	CreateSquareGrid(rArea, reso_x, reso_y, rDynamicArea);
	float r = (rArea->width > rArea->height)?(rArea->height/2):(rArea->width/2);
	r = r * r;//sqrt���g�������Ȃ��̂łQ�悵���l�Ŕ�r
	float cx = rArea->x + rArea->width / 2;
	float cy = rArea->y + rArea->height / 2;
	for (int i = 0; i < m_nodes; i ++) {
		float nr;
		nr = (cx - m_node[i].pos_st.x) * (cx - m_node[i].pos_st.x)
		   + (cy - m_node[i].pos_st.y) * (cy - m_node[i].pos_st.y);
		if ( nr > r ) {
			DeleteNode(i);
		}
	}

	return TRUE;
}

namespace N_EGM{
	static CvScalar color_table[256];
	static int color_table_enabled=FALSE;
	
	void CreateDisplayColor() {
		int lv;

		color_table[0] = CV_RGB(64,64,128);

		for(lv=1; lv<64; lv++)//�`���F
			color_table[lv] = CV_RGB(0,(lv<<2),255);

		for(lv=64; lv<128; lv++)//���F�`��
			color_table[lv] = CV_RGB(0,255,255-((lv-64)<<2));

		for(lv=128; lv<192; lv++)//�΁`���F
			color_table[lv] = CV_RGB(((lv-128)<<2),255,0);

		for(lv=192; lv<255; lv++)//���F�`��
			color_table[lv] = CV_RGB(255,255-((lv-192)<<2),0);

		color_table[255] = CV_RGB(255,0,255);
	}

	CvScalar GetDisplayColor(int lv) {
		if ( ! color_table_enabled) {
			CreateDisplayColor();
			color_table_enabled = TRUE;
		}
		if ( lv < 0 ) lv = 0; else if ( lv > 255 ) lv = 255;
		return color_table[lv];
	}

}

// �O���̊p�̃m�[�h�������
int C_GRID::CreateRoundCornerSquareGrid(S_RECT *rArea, int reso_x, int reso_y, S_RECT *rDynamicArea)
{
	CreateSquareGrid(rArea, reso_x, reso_y, rDynamicArea);
	DeleteNode(0);
	DeleteNode(reso_x);
	DeleteNode(reso_y * (reso_x + 1));
	DeleteNode(reso_x + reso_y * (reso_x+1));

	return TRUE;
}
//
//CvPoint S_POINTtoCvPoint(S_POINT p, int w, int h) {
//	return cvPoint((int)(p.x * w), (int)(p.y * h));
////	return cvPoint(cvRound(p.x * w), cvRound(p.y * h));
//}
//
//CvPoint S_POINTtoCvPoint(S_POINT p, IplImage *pImg) {
//	return cvPoint((int)(p.x * pImg->width), (int)(p.y * pImg->height));
////	return cvPoint(cvRound(p.x * pImg->width), cvRound(p.y * pImg->height));
//}
//
//S_POINT CvPointtoS_POINT(CvPoint p, IplImage *pImg) {
//	return s_point((EGM_VALUETYPE)p.x / pImg->width, (EGM_VALUETYPE)p.y / pImg->height);
//}
//
//S_POINT CvPointtoS_POINT(CvPoint p, int w, int h) {
//	return s_point((EGM_VALUETYPE)p.x / w, (EGM_VALUETYPE)p.y / h);
//}

// st-dy�Ԃ���Ō���
int C_GRID::DrawGridB(IplImage *pImg, int dy, CvScalar nodecolor, CvScalar linkcolor, CvScalar lockedcolor, CvScalar motioncolor)
{
	if ( m_nodes < 1 ) return FALSE;

	if ( ! DrawGrid(pImg, dy, nodecolor, linkcolor, lockedcolor)) return FALSE;

	int i;
	for (i = 0; i < m_nodes; i ++) {
		if ( m_node[i].pos_st.x != m_node[i].pos_dy.x
		  || m_node[i].pos_st.y != m_node[i].pos_dy.y )
		  cvLine(pImg
			, S_POINTtoCvPoint(m_node[i].pos_st, pImg)
			, S_POINTtoCvPoint(m_node[i].pos_dy, pImg)
			, motioncolor, 2);
	}

	return TRUE;
}

// �i�q��`��
int C_GRID::DrawGrid(IplImage *pImg, int dy, CvScalar nodecolor, CvScalar linkcolor, CvScalar lockedcolor, int showvalue)
{
	if ( m_nodes < 1 ) return FALSE;

	// �����N�͑��݂ɂȂ��Ă���̂�
	// ���̂܂ܕ`�����Ƃ���Ɠ�������2�x�����Ă��܂�
	// �\���̂̃R�s�[���Ƃ��đ��葤�̃����N�������Ȃ���`���Ă���
	SDLNODE *tmpnode = new SDLNODE[m_nodes];
	memcpy(tmpnode, m_node, sizeof(SDLNODE) * m_nodes);

	int i, j, k;
	CvPoint p1, p2;
	for (i = 0; i < m_nodes; i ++) {
		if ( ! tmpnode[i].active ) continue;
		for (j = 0; j < EGM_MAXLINKS; j ++) {
			if (tmpnode[i].link[j] < 0) continue;

			// �����N����ŕ`��
			if (dy) {
				p1 =  S_POINTtoCvPoint(tmpnode[i].pos_dy, pImg);
				p2 =  S_POINTtoCvPoint(tmpnode[tmpnode[i].link[j]].pos_dy, pImg);
			} else {
				p1 =  S_POINTtoCvPoint(tmpnode[i].pos_st, pImg);
				p2 =  S_POINTtoCvPoint(tmpnode[tmpnode[i].link[j]].pos_st, pImg);
			}

			if (tmpnode[i].lock || tmpnode[tmpnode[i].link[j]].lock )
				cvLine(pImg, p1, p2, lockedcolor);
			else
				cvLine(pImg, p1, p2, linkcolor);

			// ���葤�̎����ւ̃����N��T���ď���
			for (k = 0; k < EGM_MAXLINKS; k ++) {
				if (tmpnode[tmpnode[i].link[j]].link[k] == i) {
					tmpnode[tmpnode[i].link[j]].link[k] = -1;
					break;
				}
			}

		}
	}

	//�d�ݕ\���p�A�ő�l�T��
	float wm=0;
	if (showvalue==2) {
		for (i = 0; i < m_nodes; i ++) {
			if ( ! tmpnode[i].active ) continue;
			if (tmpnode[i].weight > wm)
				wm = tmpnode[i].weight;
		}		
	}

	// �m�[�h��3x3�s�N�Z���̋�`�ŕ`��
	for (i = 0; i < m_nodes; i ++) {
		if ( ! tmpnode[i].active ) continue;
		if (dy) {
			p1 = S_POINTtoCvPoint(tmpnode[i].pos_dy, pImg) + cvPoint(-1, -1);
			p2 = S_POINTtoCvPoint(tmpnode[i].pos_dy, pImg) + cvPoint( 1,  1);
		} else {
			p1 = S_POINTtoCvPoint(tmpnode[i].pos_st, pImg) + cvPoint(-1, -1);
			p2 = S_POINTtoCvPoint(tmpnode[i].pos_st, pImg) + cvPoint( 1,  1);
		}
		if (tmpnode[i].lock)
			cvRectangle(pImg, p1, p2, lockedcolor);
		else {
			if (showvalue==1) {
				int r = (int)(255 * tmpnode[i].value);
				cvRectangle(pImg, p1, p2, N_EGM::GetDisplayColor(r), 2);
			} else if (showvalue==2 && wm!=0) {
				int r = (int)(255 * tmpnode[i].weight / wm);
				cvRectangle(pImg, p1, p2, N_EGM::GetDisplayColor(r), 2);
			} else
				cvRectangle(pImg, p1, p2, nodecolor);
		}
	}

	delete [] tmpnode;

	return TRUE;
}

C_EGM::C_EGM()
{
	m_grid = NULL;
	m_jet_st = NULL;
	m_jet_dy = NULL;
	m_grid_backup = new C_GRID;
	m_grid = new C_GRID;
	m_th_lock = 0;
	m_superreso = 0;
}

C_EGM::~C_EGM()
{
	if (m_grid_backup) {
		delete m_grid_backup;
		m_grid_backup = NULL;
	}
	if (m_grid) {
		delete m_grid;
		m_grid = NULL;
	}

	Release();
}

void C_EGM::Release()
{
	if ( m_grid ) {
		delete m_grid;
		m_grid = NULL;
	}

	if ( m_jet_st ) {
		delete [] m_jet_st;
		m_jet_st = NULL;
	}

	if ( m_jet_dy ) {
		delete [] m_jet_dy;
		m_jet_dy = NULL;
	}
}

int C_EGM::SetParam(EGM_VALUETYPE weight_sim, EGM_VALUETYPE weight_dist)
{
	m_weight_sim = weight_sim;
	m_weight_dist = weight_dist;

	return TRUE;
}

// �i�q���i�[�i������Reset���Ăяo�����Ƃɒ��Ӂj
int C_EGM::SetGrid(C_GRID* from)
{
	if (m_grid_backup->Copy(from)) {
		if (! m_superreso)
			m_grid_backup->Render(m_width_st, m_height_st, m_width_dy, m_height_dy);
		Reset();
		return TRUE;
	}
	return FALSE;
}

int C_EGM::SetJet_st(IplImage** img, int images, int dy)
{
	size_t datalen;
	int ws, x, y, i, idx_src, idx_dst;
	int yw;
	EGM_VALUETYPE *ptr_dst, *ptr_src;
	int w, h;

	m_jetdim = images;
	w = img[0]->width;
	h = img[0]->height;
	ws = img[0]->widthStep / sizeof(EGM_VALUETYPE);

	datalen = sizeof(EGM_VALUETYPE) * w * h * images;

	// ����dy��dy��, st���ǂ���ɏ������ނ����߂�
	// ptr_dest�ɓ����|�C���^��ς��邱�Ƃɂ���ď������ݐ��ς��Ă���
	ptr_dst = new EGM_VALUETYPE[datalen];

	for (i = 0; i < m_jetdim; i ++) {
		ptr_src = (EGM_VALUETYPE*)img[i]->imageData;
		for (y = 0; y < h; y ++) {
			yw = y * w;
			for (x = 0; x < w; x ++) {
				idx_dst = (x + yw) * m_jetdim + i;
				idx_src = y * ws + x;
				ptr_dst[idx_dst] = ptr_src[idx_src];
			}
		}
	}

	if ( dy ) {
		if (m_jet_dy) delete [] m_jet_dy;
		m_jet_dy = ptr_dst;
		m_width_dy = w;
		m_height_dy = h;
	} else {
		if (m_jet_st) delete [] m_jet_st;
		m_jet_st = ptr_dst;
		m_width_st = w;
		m_height_st = h;
	}

	return TRUE;
}

int C_EGM::SetJet_st_b(IplImage* img, int images, int dy)
{
	size_t datalen;
	int ws, x, y, i, idx_src, idx_dst;
	int yw, yws, istep;
	EGM_VALUETYPE *ptr_dst, *ptr_src;
	int w, h;

	m_jetdim = images;
	w = img->width;
	h = img->height/images;//�c�ɂȂ����Ă���̂Ŗ����Ŋ���
	ws = img->widthStep / sizeof(EGM_VALUETYPE);

	datalen = sizeof(EGM_VALUETYPE) * w * h * images;

	// ����dy��dy��, st���ǂ���ɏ������ނ����߂�
	// ptr_dest�ɓ����|�C���^��ς��邱�Ƃɂ���ď������ݐ��ς��Ă���
	ptr_dst = new EGM_VALUETYPE[datalen];
	ptr_src = (EGM_VALUETYPE*)img->imageData;

	for (i = 0; i < m_jetdim; i ++) {
		istep = i * ws * h;
		for (y = 0; y < h; y ++) {
			yw = y * w;
			yws = y * ws + istep;
			for (x = 0; x < w; x ++) {
				idx_dst = (x + yw) * m_jetdim + i;
				idx_src = x + yws;
				ptr_dst[idx_dst] = ptr_src[idx_src];
			}
		}
	}

	if ( dy ) {
		if (m_jet_dy) delete [] m_jet_dy;
		m_jet_dy = ptr_dst;
		m_width_dy = w;
		m_height_dy = h;
	} else {
		if (m_jet_st) delete [] m_jet_st;
		m_jet_st = ptr_dst;
		m_width_st = w;
		m_height_st = h;
	}

	return TRUE;
}

int C_EGM::GetGrid(C_GRID* grid)
{
	if (! m_superreso)
		m_grid->UnRender(m_width_st, m_height_st, m_width_dy, m_height_dy);
	return grid->Copy(m_grid);
}

int C_EGM::DrawGrid(IplImage *img, int dy, CvScalar nodecolor, CvScalar linkcolor, CvScalar lockedcolor)
{
	if (! m_superreso)
		m_grid->UnRender(m_width_st, m_height_st, m_width_dy, m_height_dy);
	return m_grid->DrawGrid(img, dy, nodecolor, linkcolor, lockedcolor);
}

// EGM�̏�Ԃ�������
void C_EGM::Reset()
{
	m_lastmovednodes = 0;
	m_value = 0;
	m_LoopCount = 0;
	m_grid->Copy(m_grid_backup);
	m_grid->ResetValue();
}

// �w���EGM�����s�C���s���ꂽ�񐔂�Ԃ�
int C_EGM::EGM(int times, int range)
{
	int cnt = 0;
	while (cnt < times)
	{
		if (EGMMain(range) <= 0)
			break; //�G���[�܂��͑S�Ẵm�[�h���s���̂Ƃ��ɐ���
		cnt ++;
	}
	return cnt;
}

// 1���EGM�v�Z�����s
int C_EGM::EGMMain(int range)
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
	S_POINT s_pos_dy;

	// �f�[�^�Q�Ɨp
	int idx_st, idx_dy;

	// �G�l���M�v�Z�p
	EGM_VALUETYPE norm_dy, norm_st;
	EGM_VALUETYPE en_jetsim, en_jetdist, en_node, en_all = 0;
	EGM_VALUETYPE en_node_winner;
	const EGM_VALUETYPE const_unti_vibration = 0.001f;
	int idx_winner;

	// �S�Ẵm�[�h�ɂ��ă��[�v
	for (iNode = 0; iNode < nodes; iNode ++) {

		if (node_now[iNode].active == FALSE) continue;

		// ���ݒ��ڂ��Ă���m�[�h��static�����W���擾
		pos_st = S_POINTtoCvPoint(node_now[iNode].pos_st, m_width_st, m_height_st);

		if(pos_st.x < 0) continue;
		if(pos_st.x >= m_width_st)  continue;
		if(pos_st.y < 0) continue;
		if(pos_st.y >= m_height_st) continue;

		idx_st = (pos_st.y * m_width_st + pos_st.x) * m_jetdim;

		// �ő�l�T���p�ϐ��̏�����
		en_node_winner = 0;
		idx_winner = 0;

		// dynamic���̒��ڍ��W��ς��Ȃ���G�l���M�ő�̓_��T��
		for(iDelta = 0; iDelta < delta_len; iDelta++) {

			// ���ݒ��ڂ��Ă���m�[�h��dynamic�����W���擾
			pos_dy = deltaxy[iDelta] + S_POINTtoCvPoint(node_now[iNode].pos_dy, m_width_dy, m_height_dy);
			s_pos_dy = CvPointtoS_POINT(pos_dy, m_width_dy, m_height_dy);

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

				// ���ꂪ0�ɂȂ�悤����������ς̍Œ�l���Ⴍ(-2)����
				if( norm_dy == 0 || norm_st == 0)
					en_jetsim = (EGM_VALUETYPE)-2;
				else
					en_jetsim = en_jetsim / (norm_st * norm_dy);
			} else {
				en_jetsim = 1.0f - fabs(m_jet_st[idx_st] - m_jet_dy[idx_dy]) / 255.0f;
			}
			// �i�q�̘c�݂��v�Z -> en_jetdist
			en_jetdist = 0;

			// �ĂP''�F�i�q�̈ړ������@|�Œ葤�����N - �ړ��������N|/�Œ葤�����N
			{
				//�����N�̐��𐔂���
				int Lc=0;
				EGM_VALUETYPE v=0;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X
					S_POINT vLink_dy = node_now[n].pos_dy - s_pos_dy;
					S_POINT vLink_st = node_now[n].pos_st - node_now[iNode].pos_st;

					//
					//v += s_size(vLink_st - vLink_dy) / s_size(vLink_st);
					EGM_VALUETYPE f = s_size(vLink_st - vLink_dy) / s_size(vLink_st);
					v += f * f;

					Lc++;//�����N�̐��𐔂��Ă���
				}

				// �c�ݓx = 
				en_jetdist += v / Lc;
			}

			//�ĂQ'�F�����N�̃x�N�g����st, dy�����ꂼ��ō��
			//1-���ρi�̕��ρj��c�ݓx�Ƃ���
			{
				//�����N�̐��𐔂���
				int Lc=0;
				EGM_VALUETYPE v=0;
				int n;

				for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
					if (node_now[iNode].link[iLink] < 0) continue;

					n = node_now[iNode].link[iLink];//�����N��̃C���f�b�N�X
					S_POINT vLink_dy = node_now[n].pos_dy - s_pos_dy;
					S_POINT vLink_st = node_now[n].pos_st - node_now[iNode].pos_st;

					// ���ς��Ƃ�
					v += vLink_dy * vLink_st;

					Lc++;//�����N�̐��𐔂��Ă���
				}

				// �c�ݓx = 1 - ���ς̕���
				en_jetdist += 1.0f - v / Lc;

			}

			//// �ĂP�F�i�q�̈ړ������@|�Œ葤���W - �ړ������W|
			//{
			//	en_jetdist += s_size(node_now[iNode].pos_st - s_pos_dy);
			//}
	
			//// �ĂP'�F���k�o�l�i�m�[�h���m���d�Ȃ����Ƃ���c�ݍő�(-1.0)�Cst�Ɠ���������0�Cst�̔{�̒����ł��c�ݍő�Ƃ���j
			////���ڃm�[�h�ƃ����N��m�[�h�Ԃ̃x�N�g�����Ƃ�
			//{
			//	//�����N�̐��𐔂���
			//	int Lc=0;
			//	for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
			//		if (node_now[iNode].link[iLink] < 0) continue;
			//		Lc++;
			//	}

			//	//
			//	float sum_dist=0, len_st, len_dy;
			//	for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
			//		if (node_now[iNode].link[iLink] < 0) continue;
			//		int n = node_now[iNode].link[iLink];
			//		len_st = s_size(node_now[n].pos_st - node_now[iNode].pos_st);
			//		len_dy = s_size(node_now[n].pos_dy - s_pos_dy);
			//		if (len_st>0) {
			//			if (len_dy < len_st)
			//				sum_dist += (len_st - len_dy) / len_st;
			//			else
			//				sum_dist += (len_dy - len_st) / len_st;
			//		}
			//	}
			//	if (Lc>0)
			//		en_jetdist += sum_dist / Lc;
			//}

			//// �ĂQ�F�����N�Ԃ̊p�x����ׂ��x�N�g�������Cst�Cdy�Ԃœ��ς��Ƃ�
			//{
			//	//�����N�̐��𐔂���
			//	int Lc=0;
			//	for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
			//		if (node_now[iNode].link[iLink] < 0) continue;
			//		Lc++;
			//	}
			//	if (Lc>1) {

			//		S_POINT *vLink_dy = new S_POINT [Lc];
			//		S_POINT *vLink_st = new S_POINT [Lc];
			//		float *vrot_dy = new float[Lc];
			//		float *vrot_st = new float[Lc];
			//		int i, n, L=0;

			//		//���ڃm�[�h�ƃ����N��m�[�h�Ԃ̃x�N�g�����Ƃ�
			//		for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
			//			if (node_now[iNode].link[iLink] < 0) continue;
			//			n = node_now[iNode].link[iLink];
			//			vLink_dy[L] = node_now[n].pos_dy - s_pos_dy;
			//			vLink_st[L] = node_now[n].pos_st - node_now[iNode].pos_st;
			//			L++;
			//		}

			//		// �����N�Ԃ̊p�x(����)�����߂�
			//		int i2;
			//		for (i=0;i<Lc;i++) {
			//			i2 = (i+1) % Lc;
			//			vrot_dy[i] = ((vLink_dy[i]*vLink_dy[i2])+1.0f)/2.0f;//����(-1~1)�������0~1�ɐ��K��
			//			vrot_st[i] = ((vLink_st[i]*vLink_st[i2])+1.0f)/2.0f;
			//		}

			//		// ����ɁCst-dy�Ԃœ��ς����
			//		float dot=0,sumdy=0,sumst=0;
			//		for (i=0;i<Lc;i++) {
			//			dot += vrot_dy[i]*vrot_st[i];
			//			sumdy += vrot_dy[i]*vrot_dy[i];
			//			sumst += vrot_st[i]*vrot_st[i];
			//		}

			//		if ( sumdy > 0 && sumst > 0 )
			//			dot = dot / ( EGM_SQRT(sumdy) * EGM_SQRT(sumst) );
			//		else
			//			dot = 0;

			//		// �c��ł��Ȃ����dot==1.0�̂͂��Ȃ̂ŁC�P��菬�������(1-dot)/2��c�ݓx�Ƃ���
			//		en_jetdist += (1.0f - dot)/2.0f;
			//		if (dot<1.0f)
			//			dot=0;
			//		delete [] vLink_dy;
			//		delete [] vLink_st;
			//		delete [] vrot_dy;
			//		delete [] vrot_st;
			//	}
			//}

			// �c�݌v�Z
			//if (node_now[iNode].lock == FALSE) {
			//	EGM_VALUETYPE xsum=0, ysum=0;
			//	int n;
			//	for (iLink = 0; iLink < EGM_MAXLINKS; iLink ++) {
			//		n = node_now[iNode].link[iLink];
			//		if (n < 0) continue;
			//		xsum += node_now[n].pos_dy.x - pos_dy.x;
			//		ysum += node_now[n].pos_dy.y - pos_dy.y;
			//	}
			//	en_jetdist = EGM_SQRT(xsum * xsum + ysum * ysum);
			//} else
			//	en_jetdist = 0;

			// �d�݂����ăm�[�h�̃G�l���M���v�Z -> en_node
			en_node = m_weight_sim * en_jetsim - m_weight_dist * en_jetdist;

			// �ő�l���Ƃ����G�l���M���L��
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
			node_next[iNode].pos_dy = CvPointtoS_POINT(
										deltaxy[idx_winner]
										+ S_POINTtoCvPoint(node_now[iNode].pos_dy, m_width_dy, m_height_dy)
										, m_width_dy
										, m_height_dy);
			movednodes ++;
		}

		// �G�l���M���L��
		node_next[iNode].value = en_node_winner;

		// ���̃m�[�h�̃G�l���M��S�̂̃G�l���M�ɉ��Z
		en_all += en_node_winner;

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

	int c = m_grid->GetActiveNodeCount();
	if (c > 0)
		m_value /= c;

	if (m_weight_sim != 0)
		m_value /= m_weight_sim;

	m_lastmovednodes = movednodes;

	// ���I�z��̉��
	delete [] deltaxy;

	// �ړ������m�[�h�̐���Ԃ�
	return movednodes;
}

// �T�u���[�`��
// �摜�t�H�[�}�b�g�ϊ�
// OpenCV2.0�̂��̂��v���ʂ蓮���Ȃ��̂Ŏ����ō쐬
void cvt_8U1_to_32F1(IplImage *src, IplImage *dst)
{
	float *dstdata = (float*)dst->imageData;
	//float *dstdata2 = (float*)dst->imageDataOrigin;
	int ws_dst = (int)(dst->widthStep / sizeof(float));
	unsigned char *srcdata = (unsigned char*)src->imageData;
	//unsigned char v;

	for (int j = 0; j < src->height; j++){
		for (int i = 0; i < src->width; i++){
			dstdata[i + j * ws_dst] = (float)srcdata[i + j * src->widthStep];
			//dstdata2[i + j * dst->width] = (float)srcdata[i + j * src->widthStep];
		}
	}
}

void cvt_8U1_to_8U3(IplImage *src, IplImage *dst)
{
	int ws_dst = dst->widthStep;
	int ws_src = src->widthStep;
	unsigned char *srcdata = (unsigned char*)src->imageData;
	unsigned char *dstdata = (unsigned char*)dst->imageData;
	int pd, ps;

	for (int j = 0; j < src->height; j++){
		for (int i = 0; i < src->width; i++){
			ps = j * ws_src + i;
			pd = j * ws_dst + (i * 3);
			dstdata[pd   ] = srcdata[ps];
			dstdata[pd + 1] = srcdata[ps];
			dstdata[pd + 2] = srcdata[ps];
		}
	}
}

void cvt_32F1_to_8U3(IplImage *src, IplImage *dst)
{
	float *srcdata = (float*)src->imageData;
	int ws_src = (int)(src->widthStep / sizeof(float));
	unsigned char *dstdata = (unsigned char*)dst->imageData;
	//unsigned char *dstdata2 = (unsigned char*)dst->imageDataOrigin;
	unsigned char v;
	double ve;
	int p;

	for (int j = 0; j < src->height; j++){
		for (int i = 0; i < src->width; i++){
			//double ve = cvGetReal2D(src, i, j);
			ve = srcdata[i + j * ws_src];

			v = (unsigned char)cvRound(ve);
			//cvSetReal2D( (IplImage*)dst, j, i, ve );
			p = i * 3 + j * dst->widthStep;
			dstdata[p + 0] = v;
			dstdata[p + 1] = v;
			dstdata[p + 2] = v;
			//p = (i + j * dst->width) * 3;
			//dstdata[p + 0] = v;
			//dstdata[p + 1] = v;
			//dstdata[p + 2] = v;
		}
	}
}

// �ʐϊg��
void srExtendArea(S_RECT *r, float per, int correct_over)
{
	EGM_VALUETYPE cx, cy, neww, newh;
	cx = r->x + r->width / 2;
	cy = r->y + r->height / 2;
	neww = r->width * per;
	newh = r->height * per;
	r->x = cx - neww / 2;
	r->y = cy - newh / 2;
	r->width = neww;
	r->height = newh;
	if (! correct_over) return;
	if (r->x < 0) r->x=0;
	if (r->y < 0) r->y=0;
	if (r->width+r->x > 1.0f) r->width=1.0f-r->x;
	if (r->height+r->y > 1.0f) r->height=1.0f-r->y;
}
