#ifndef __LANE_DETECT_H__
#define __LANE_DETECT_H__

//#include "../fcws/common.h"
#ifdef __cplusplus
extern "C" {
#include "../utils/ma.h"
}
#endif

#include "../utils/common.hpp"
#include "../utils/imgproc.hpp"

#define VALID_AGENT_TRIANGLE_AREA    30
#define VALID_AGENT_DISTANCE         60.0
#define VALID_AGENT_HORIZONTAL       30 
#define VALID_DIST_OF_K_TO_VECTOR_IJ 2
#define VALID_DIST_OF_K_TO_KLUGE_POLY   2
#define VALID_DIST_OF_POINT_TO_POINT 40   
#define MAX_FIND_AGENT_FAIL_COUNT    400
#define MAX_FIND_LINE_FAIL_COUNT     30
#define VALID_LINE_EVIDENCE_PERCENTAGE  0.20
#define VALID_POINT_COUNT            30

#define MAX_LANE_NUMBER 5

class CLDWS;


enum {
    LANE_DETECT_CHECK_OK = 1,
    LANE_DETECT_CHECK_AGENT_TRIANGLE_AREA_ERROR = -1,
    LANE_DETECT_CHECK_AGENT_DISTANCE_ERROR = -2,
    LANE_DETECT_CHECK_HORIZONTAL_ERROR = -3,
    LANE_DETECT_CHECK_DIST_OF_K_TO_VECTOR_IJ_ERROR = -4,
    LANE_DETECT_CHECK_DIST_OF_K_TO_IJ_ERROR = -5,
};

enum {
    LANE_LEFT = 0,
    LANE_RIGHT,
    LANE_CENTER,
    LANE_NUM
};

enum LaneType{
    LaneTypeLeft = 0,
    LaneTypeRight,
    LaneTypeBoth,
    LaneTypeNon,
    LaneTypeNum
};

typedef struct lanepoint{
    int r;
    int c;
    int pickup;
    int pol;
}lanepoint;

typedef struct lane{
    struct{
        double k;
        double b;
        double v;
    }kluge_poly;
	struct{
		float a;
		float b;
	}line_poly;
    int     trust;
    int     trust_count;
    lanepoint   *pix;
    int     pix_count;
    int     pix_col_center;
    int     shift;
    int     exist;
    int     fail_count;
    double  evidence_percentage;
    double  grade;
}lane;

#define LANE_FAIL_COUNT 5
#define LANE_MAX_STATUS_NUM 30
#define LANE_OVER_CENTER_TH 48//90
#define LANE_OVER_SIDE_TH   90//70
#define LANE_FAR_AWAY_LEFT_SIDE 80
#define LANE_FAR_AWAY_RIGHT_SIDE    560

typedef struct lane_status{
	int		frame;
	char    	cur;
	struct  status{
		int	frame;
		enum    LaneType   type;
		int	dist;
		char	alert;
	}st[LANE_MAX_STATUS_NUM];
}lane_status;

typedef struct ldws_stat_s{
    lanepoint   p[6];
    lane        *l[LANE_NUM];
    lane_status ls;
    int         alert;    

	lanepoint 	intersection_point;	//for vanishing line detect.
	int			widthofbottom;
}lane_stat_t;
 
typedef struct param_s {
    CLDWS*  ldws;
    int     id;
}param_t;

class CLDWS {
    protected:
        bool        m_terminate;
        CImgProc*   m_ip; // Image Process object
        gsl_matrix* m_imgy;
        gsl_matrix* m_edge_imgy;
        
        lane_stat_t m_lane_stat;

        uint8_t     m_thread_done;
        param_t     m_param[2];
        pthread_t   m_thread[2];
        pthread_mutex_t m_mutex[2];
        pthread_cond_t  m_cond[2];
        pthread_mutex_t m_jobdone_mutex[2];
        pthread_cond_t  m_jobdone_cond[2];

    public:
        CLDWS();

        ~CLDWS();

        bool Init();

        bool DeInit();

        bool DoDetection(uint8_t* src, int linesize, int w, int h);

        bool GetEdgedImg(uint8_t* dst, int w, int h, int linesize);

    protected:
        double agent_cal_dist(lanepoint* p1, lanepoint* p2);

        int agent_determine_ijk(lanepoint* p1, lanepoint* p2, lanepoint* p3, lanepoint** i, lanepoint** j, lanepoint** k);

        int check_agent_is_horizontal(lanepoint* i, lanepoint* j);

        int check_dist_of_k_to_vector_ij(lanepoint* i, lanepoint* j, lanepoint* k);

        int check_dist_of_k_to_ij(lanepoint* i, lanepoint* j, lanepoint* k);

        int check_agent_valid(lanepoint** p1, lanepoint** p2, lanepoint** p3);

        static void* FindPartialLane(void* args);

        bool FindLane(gsl_matrix* src,
                    int start_row,
                    int start_col,
                    lanepoint* p,
                    lane *l[LANE_NUM]);
};

//void LDW_DoDetection(uint8_t* src, int linesize, int w, int h);
//
//void LDW_DeInit(void);

//void lane_detect(unsigned char *img, int rows, int cols, int start_row, int start_col, point *p, lane **l);
//int lane_check_over_status(lane **l, int center_pos);
#endif
