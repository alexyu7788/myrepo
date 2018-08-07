#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>

#include "models2.pb-c.h"

#define MAX_FN_L    255

static const char *search_vehicle_model_pattern[FCWS__VEHICLE__MODEL__TYPE__TOTAL] = {"Compact", "Middle", "Large", "SUV", "Bus", "Truck", "Motocycle", "Bike"};
static const char *search_local_model_pattern[FCWS__LOCAL__TYPE__TOTAL] = {"Left", "Right", "Center"};

unsigned long long GetTime();
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
