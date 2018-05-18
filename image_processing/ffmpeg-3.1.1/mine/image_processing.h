#ifndef __IMAGE_PROCESSING_H__
#define __IMAGE_PROCESSING_H__

#define EDGEDET_MASK_X    0
#define EDGEDET_MASK_Y    1
#define EDGEDET_MASK_XY   2

#if 0
int imp_LoadBMP(int width, int height, unsigned char **rgb, const char *fn);
void imp_ConvertRGB2YUV(int width, int height, unsigned char **rgb, unsigned char **yuv);
int imp_ConvertYUV2BMP(int width, int height, unsigned char **yuv, unsigned char **rgbonly_y);
int imp_CropRGB(unsigned char *src, unsigned char **des, int width, int height, int crop_width, int crop_height, int start_x, int start_y, int crop_x_shift, int crop_y_shift);
void imp_sobel(unsigned char *buf, int type, int rows, int cols, unsigned char **edged, unsigned char **edged_y);
#endif
#endif
