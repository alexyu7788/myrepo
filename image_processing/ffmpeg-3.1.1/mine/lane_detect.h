#ifndef __LANE_DETECT_H__
#define __LANE_DETECT_H__

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

typedef struct point{
    int r;
    int c;
    int pickup;
    int pol;
}point;

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
    point   *pix;
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
    point       p[6];
    lane        *l[LANE_NUM];
    lane_status ls;
    int         alert;    

	point 		intersection_point;	//for vanishing line detect.
	int			widthofbottom;
}lane_stat_t;

//void lane_detect(unsigned char *img, int rows, int cols, int start_row, int start_col, point *p, lane **l);
//int lane_check_over_status(lane **l, int center_pos);
#endif
