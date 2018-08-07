/*
 * =====================================================================================
 *
 *       Filename:  gen_param.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/10/2013 03:32:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <vector>
#include <math.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
//#include <cjson/cJSON.h>
#include "pca.h"
#include "utility.h"
#include "models2.pb-c.h"
#include "fcws.h"

using namespace std;

int main(int argc, char *argv[])
{
    int i,j;
//    FILE *list = NULL;
//    char src[MAX_FN_L] = {0};
//    int list_cnt = 0;
//    uint8_t **img = NULL;
//    uint8_t *img_mean = NULL;
//    int16_t **covar = NULL;

    int ch;
    int file_cnt = 0;
    int w = 0,h = 0, first_k_coms = 0;
    char *src_folder = 0, *dst_folder = 0, output_folder[MAX_FN_L] = {0}, temp_buf[MAX_FN_L] = {0};
    char **src_file_list = 0;
    char filepath[MAX_FN_L];
    struct dirent *entry;
    DIR *dir = NULL;
    struct stat st;
    vector<string> feedin;
    string infolder, outfolder;


    while ((ch = getopt(argc, argv, "s:d:o:k:")) != -1)
    {
    	switch (ch)
    	{
    	case 's':
    		src_folder = optarg;
    		infolder = optarg;
    		break;
    	case 'o':
    		dst_folder = optarg;
    		outfolder = optarg;
    		break;
    	case 'd':
    		sscanf(optarg, "%d:%d", &w, &h);
    		break;
    	case 'k':
    		sscanf(optarg, "%d", &first_k_coms);
    		break;
    	default:
    		exit(-1);
    	}
    }

#if 1
	FCWS *fcws = new FCWS();

	if (fcws)
	{
		if (fcws->LoadFiles(infolder))
		{
			//		models->LoadFrom(model_name);
			//		fcws->FeedFiles(feedin, w, h);
			//
					fcws->DoTraining(first_k_coms, 0, 0, 0, outfolder);
			//		fcws->SaveTo(model_name);
		}

		delete fcws;
	}
#else
    // Create a new output folder
    snprintf(output_folder, MAX_FN_L, "%s_%d_%d", dst_folder, w, h);
    if (stat(output_folder, &st) != -1)
    {
        snprintf(temp_buf, MAX_FN_L, "rm -r %s", output_folder);
    	system(temp_buf);
    }

    memset(temp_buf, 0, sizeof(char) * MAX_FN_L);
    snprintf(temp_buf, MAX_FN_L, "mkdir %s", output_folder);
    system(temp_buf);
    sync();

    // Read source yuv
    memset(temp_buf, 0, sizeof(char) * MAX_FN_L);
    snprintf(temp_buf, MAX_FN_L, "%s_%d_%d", src_folder, w, h);
    dir = opendir(temp_buf);
    
    if (dir == NULL)
    {
    	printf("Can not open %s.\n", src_folder);
    	exit(-1);
    }

    while ((entry = readdir(dir)) != NULL)
    {
    	if (entry->d_type & DT_REG)
    	{
//    		printf("%s\n", entry->d_name);
    		file_cnt++;
    	}
    }

    rewinddir(dir);
    printf("file count %d\n", file_cnt);

    src_file_list = (char**)malloc(sizeof(char*) * file_cnt);
    for (i=0 ; i<file_cnt ; i++)
    	src_file_list[i] = (char*)malloc(sizeof(char) * MAX_FN_L);

    i = 0;
    while ((entry = readdir(dir)) != NULL)
    {
    	if (entry->d_type & DT_REG)
    	{
    		snprintf(src_file_list[i], MAX_FN_L, "%s", entry->d_name);
//    		printf("%s\n", src_file_list[i]);
    		i++;
    		snprintf(filepath, MAX_FN_L, "./%s_%d_%d/%s", src_folder, w, h, entry->d_name);
    		feedin.push_back(filepath);
    	}
    }

	// mean, co-variance, eigenvalues, eigenvectors, reconstructed image and residual image of orignal images.
	{
		int ret = 0, rows, cols;
		gsl_vector *mean = NULL,
				   *EigValue = NULL;
		gsl_matrix *im_y = NULL,
				   *cov_m = NULL,
				   *EigVector = NULL,
				   *ProjectData = NULL,
				   *ReConstrcution = NULL,
				   *Residual = NULL;
		gsl_matrix_view FirstKEigVector;

		pca_t _1st_pca, _2nd_pca;
		// Load images.
		im_y = load_image_to_matrix(src_folder, src_file_list, file_cnt, w, h);

		if (im_y)
		{
//			mean 	= cal_mean(im_y);
//			cov_m 	= cal_covariance(im_y, mean);
//			ret 	= cal_eigenvalue_eigenvector(cov_m, &EigValue, &EigVector);

			if (ret == GSL_SUCCESS)
			{
				// Project the original dataset
//				rows = EigVector->size1;
//				cols = EigVector->size2;
//				printf("EigVector:\t\t rows:%d,\t cols: %d\n", rows, cols);
//
//				FirstKEigVector = get_first_k_components(EigVector, first_k_coms);
//				rows = FirstKEigVector.matrix.size1;
//				cols = FirstKEigVector.matrix.size2;
//				printf("FirstKEigVector:\t rows:%d,\t cols: %d\n", rows, cols);
//
//				ProjectData = generate_projection_matrix(&FirstKEigVector.matrix, im_y);
//
//				ReConstrcution = generate_reconstruct_image_matrix_from_pca(&FirstKEigVector.matrix, ProjectData, mean);
//				rows = ReConstrcution->size1;
//				cols = ReConstrcution->size2;
//				printf("ReConstrcution:\t rows:%d,\t cols: %d\n", rows, cols);
//
//				Residual = generate_residual_image_matrix(im_y, ReConstrcution);
////
//				// write_matrix_to_images(ReConstrcution, output_folder, src_file_list, "recontruct");
//				// write_matrix_to_images(Residual, output_folder, src_file_list, "residual");
//
//				char model_name[1024] = {0};
//				gsl_matrix *tmp1 = NULL, *tmp2 = NULL;
//
//				snprintf(model_name, 1024, "models_%d_%d.bin", (int)FirstKEigVector.matrix.size1, (int)FirstKEigVector.matrix.size2);
//				model_save(model_name, &FirstKEigVector.matrix);
//				model_load(model_name, &tmp1, &tmp2);

//				FCWS *fcws = new FCWS();
//				if (fcws && fcws->LoadFiles(infolder, outfolder))
//				{
//					models->LoadFrom(model_name);
//					fcws->FeedFiles(feedin, w, h);

//					fcws->DoTraining(first_k_coms, 0, 0, 0, output_folder);
//					fcws->SaveTo(model_name);

//					delete fcws;
//				}

//				if (tmp1 && tmp2)
//				{
////					int i,j;
////					for (i=0 ; i<tmp1->size1 ; i++)
////					{
////						for (j=0 ; j<tmp1->size2 ; j++)
////							printf("tmp1[%d][%d]=%lf\n", i, j, gsl_matrix_get(tmp1, i, j));
////					}
//
//					gsl_matrix_free(tmp1);
//					gsl_matrix_free(tmp2);
//				}

//				// JSON
//				save_to_json("json_text.json", &FirstKEigVector.matrix);
//				gsl_matrix *testm = NULL;
//				load_model_from_json_description("json_text.json", &testm);
//
//				if (testm)
//				{
////					for (j=0 ; j<testm->size2 ; j++)
////					{
////						printf("[0][%d]=%.12lf\n", j, gsl_matrix_get(testm, 0, j));
////					}
////
////					for (j=0 ; j<testm->size2 ; j++)
////					{
////						printf("[1][%d]=%.12lf\n", j, gsl_matrix_get(testm, 1, j));
////					}
//					gsl_matrix_free(testm);
//				}
				{
//					Para *para = new Para();
//					if (para)
//					{
//						para->Init(FCWS__PARA__TYPE__PCA, &FirstKEigVector.matrix);
//
//						delete para;
//					}

//					Position *pos = new Position();
//					if (pos)
//					{
//						pos->Set(FCWS__POSITION__TYPE__LEFT, FCWS__PARA__TYPE__PCA, &FirstKEigVector.matrix);
//						delete pos;
//					}

//					VehicleModel *vm = new VehicleModel();
//					if (vm)
//					{
//						vm->Set(FCWS__VEHICLE__MODEL__TYPE__Compact, FCWS__POSITION__TYPE__LEFT, FCWS__PARA__TYPE__PCA, &FirstKEigVector.matrix);
//						vm->Set(FCWS__VEHICLE__MODEL__TYPE__Compact, FCWS__POSITION__TYPE__LEFT, FCWS__PARA__TYPE__PCA, &FirstKEigVector.matrix);
//
//						delete vm;
//					}


				}
			}
		}

		if (im_y)
			gsl_matrix_free(im_y);

		if (mean)
			gsl_vector_free(mean);

		if (cov_m)
			gsl_matrix_free(cov_m);

		if (EigValue)
			gsl_vector_free(EigValue);

		if (EigVector)
			gsl_matrix_free(EigVector);

		if (ProjectData)
			gsl_matrix_free(ProjectData);

		if (ReConstrcution)
			gsl_matrix_free(ReConstrcution);

		if (Residual)
			gsl_matrix_free(Residual);

	}


    closedir(dir);

    if (file_cnt && src_file_list)
    {
		for (i=0 ; i<file_cnt; i++)
		{
			if (src_file_list[i])
				free(src_file_list[i]);
		}
		free(src_file_list);
    }

//    create_monitor();
#endif

    return 0;
}
