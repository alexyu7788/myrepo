#ifndef __CANNY_H_
#define __CANNY_H_

#define NOEDGE 255
#define POSSIBLE_EDGE 128
#define EDGE 0

extern void canny(unsigned char *image , int rows , int clos , float sigma , float tlow , float thigh , unsigned char *edge, unsigned char *edge_y);
extern void gaussian_smooth(unsigned char *image , int rows , int clos , float sigma , short int **smoothedim);
extern void make_gaussian_kernel(float sigma , float **kernel , int *windowsize);
extern void derrivative_x_y(short int *smoothedim , int rows , int cols , short int **delta_x , short int **delta_y);
extern void radian_direction(short int *delta_x, short int *delta_y, int rows,int cols, float **dir_radians, int xdirtag, int ydirtag);
extern double angle_radians(double x, double y);
extern void magnitude_x_y(short int *delta_x, short int *delta_y, int rows, int cols,short int **magnitude);
extern void non_max_supp(short *mag, short *gradx, short *grady, int nrows, int ncols,unsigned char *result);
extern void apply_hysteresis(short int *mag, unsigned char *nms, int rows, int cols,float tlow, float thigh, unsigned char *edge);
extern void follow_edges(unsigned char *edgemapptr, short *edgemagptr, short lowval,int cols);
#endif
