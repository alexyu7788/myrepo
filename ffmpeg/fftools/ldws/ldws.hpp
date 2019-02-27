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
#include "../utils/mop.h"

#define VALID_AGENT_TRIANGLE_AREA       30
#define VALID_AGENT_DISTANCE            60.0
#define VALID_AGENT_HORIZONTAL          30 
#define VALID_DIST_OF_K_TO_VECTOR_IJ    2
#define VALID_DIST_OF_K_TO_KLUGE_POLY   2
#define VALID_DIST_OF_POINT_TO_POINT    40   

#define MAX_FIND_AGENT_FAIL_COUNT       400
#define MAX_FIND_LINE_FAIL_COUNT        30
#define VALID_LINE_EVIDENCE_PERCENTAGE  0.20
#define VALID_POINT_COUNT               30

#define MAX_LANE_NUMBER 5

class CLDWS;

enum {
    LANE_DETECT_CHECK_OK = 1,
    LANE_DETECT_CHECK_AGENT_ANGLE = -1,
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

    pthread_mutex_t mutex;
}lane;

#define LANE_FAIL_COUNT 5
#define LANE_MAX_STATUS_NUM 30
#define LANE_OVER_CENTER_TH 48//90
#define LANE_OVER_SIDE_TH   90//70
#define LANE_FAR_AWAY_LEFT_SIDE 80
#define LANE_FAR_AWAY_RIGHT_SIDE    560

#define LANE_THREADS 2 

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
    CLDWS*  obj;
    int     id;
}param_t;

class CLDWS {
    protected:
        BOOL        m_terminate;

        uint32_t    m_rows;
        uint32_t    m_cols;
        uint32_t    m_rowoffset;

        uint32_t    m_left_coloffset;
        uint32_t    m_right_coloffset;

        // vanishing point
        point_t      m_vp;

        // straight lane
        float       m_slope_left;
        float       m_delta_left;
        float       m_slope_right;
        float       m_delta_right;
        
        // Image Process object
        CImgProc*   m_ip; 

        gsl_matrix* m_imgy;
        gsl_matrix* m_edged_imgy;

        lane_stat_t m_lane_stat;

        param_t         m_param[LANE_THREADS];
        pthread_t       m_thread[LANE_THREADS];
        pthread_mutex_t m_mutex[LANE_THREADS];
        pthread_cond_t  m_cond[LANE_THREADS];

        uint8_t         m_thread_done;
        pthread_mutex_t m_jobdone_mutex;
        pthread_cond_t  m_jobdone_cond;

        gsl_matrix_view m_subimgy[LANE_THREADS];

        // ------------------------DLIB---------------------------
        smatrix         m_dlib_imgy;
        smatrix         m_dlib_edged_imgy;
        smatrix         m_dlib_subimgy[LANE_THREADS];

    public:
        CLDWS();

        ~CLDWS();

        BOOL Init();

        BOOL DeInit();

        BOOL DoDetection(uint8_t* src, 
                         int w, 
                         int h, 
                         int linesize, 
                         int rowoffset,
                         BOOL crop = false);

        BOOL GetEdgedImg(uint8_t* dst, int w, int h, int linesize);

        // For drawing
        BOOL GetLane(lane** left, lane** right, lane** center);

        BOOL DestroyLane(lane** left, lane** right, lane** center);

        BOOL GetLanePoints(point_t* ltop, 
                           point_t* lbottom, 
                           point_t* rtop, 
                           point_t* rbottom,
                           point_t* ctop, 
                           point_t* cbottom
                           );

        BOOL ApplyDynamicROI(gsl_matrix* src);

        // ------------------------DLIB---------------------------
        BOOL ApplyDynamicROI(smatrix& src);

    protected:
        static lane* LaneInit(void);

        static lane* LaneInit(const lane* const ref);

        static void LaneDeinit(lane** l);

        static double agent_cal_dist(lanepoint* p1, lanepoint* p2);

        static int agent_determine_ijk(lanepoint* p1, 
                                       lanepoint* p2, 
                                       lanepoint* p3, 
                                       lanepoint** i, 
                                       lanepoint** j, 
                                       lanepoint** k);

        static int check_agent_angle(int id, lanepoint* i, lanepoint* j);

        static int check_agent_is_horizontal(lanepoint* i, lanepoint* j);

        static int check_dist_of_k_to_vector_ij(lanepoint* i, lanepoint* j, lanepoint* k);

        static int check_dist_of_k_to_ij(lanepoint* i, lanepoint* j, lanepoint* k);

        static int check_agent_valid(int id, lanepoint** p1, lanepoint** p2, lanepoint** p3);

        // ----------------kluge polynomial relevant----------------
        static void solve_equation(gsl_matrix* a, gsl_vector* x, gsl_vector* b);

        static void kp_solve(lanepoint* pp[3], lane* l); 

        static double kp_dist_of_points_to_kp(lanepoint* p, lane* l, uint32_t search_range_r);

        static double kp_evidence_check(int count, lanepoint* p, lane*l);

        static void kp_gen_point(uint32_t rows, 
                                 uint32_t cols, 
                                 lane* l, 
                                 uint32_t rowoffset = 0, 
                                 uint32_t coloffset = 0);

        static void kp_gen_center_point(uint32_t rows, 
                                        uint32_t cols, 
                                        lane* left, 
                                        lane* right, 
                                        lane* center);

        static void kp_rel_point(lane* l);

        void WakeUpThread();

        void WaitThreadDone();

        BOOL UpdateLaneStatus(uint32_t rows, 
                              uint32_t cols, 
                              lane* left, 
                              lane* right, 
                              lane* center);

        BOOL CalStraightLanesPoly(void);

        BOOL CalVanishingPoint(void);

        static void* FindPartialLane(void* args);

        BOOL FindLane(gsl_matrix* src,
                    int start_row,
                    int start_col,
                    lanepoint* p,
                    lane *l[LANE_NUM]);
        // ------------------------DLIB---------------------------
        BOOL FindLane(smatrix& src,
                    int start_row,
                    int start_col,
                    lanepoint* p,
                    lane *l[LANE_NUM]);
};

#endif
