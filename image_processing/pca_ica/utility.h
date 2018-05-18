#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
//#include <cjson/cJSON.h>
//#include <cjson/cJSON_Utils.h>

#define MAX_FN_L    255

unsigned long long get_time();
void print_matrix(char *name, gsl_matrix *m);
void print_vector(char *name, gsl_vector *v);
void print_matrix_float(char *name, gsl_matrix_float *m);
void print_vector_float(char *name, gsl_vector_float *v);
void write_matrix_to_images(gsl_matrix *x, char *dst_folder, char **file_list, char *postfix);
gsl_matrix *load_image_to_matrix(char *src_folder, char **file_list, int file_count, int w, int h);
//int save_to_json(char *json_filename, gsl_matrix *m);
//int load_model_from_json_description(char *json_filename, gsl_matrix **m);
int model_save(char *model_name, gsl_matrix *m);
int model_load(char *model_name, gsl_matrix **m1, gsl_matrix **m2);
#endif
