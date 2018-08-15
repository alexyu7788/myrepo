#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "localfeature.h"
#include "utility.h"

#define MAX_ITERATIONS	1000
#define TOLERANCE		0.0001
#define ALPHA			1

CLocalFeature::CLocalFeature(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type)
{
    m_terminate = false;

	m_output_folder.clear();

	m_vm_name = strdup(search_vehicle_model_pattern[vm_type]);
	m_vm_type = vm_type;

	m_local_name = strdup(search_local_model_pattern[local_type]);
	m_local_type = local_type;

	m_para_count = 0;

	for (int i=FCWS__PARA__TYPE__PCA ; i<FCWS__PARA__TYPE__TOTAL ; i++)
	{
		m_para[i] = new CParam((FCWS__Para__Type)i);
	}

    m_has_param                 = false;
    m_para2                     = new CParam2();

	m_pca_first_k_components 	=
	m_pca_compoments_offset 	=
	m_ica_first_k_components 	=
	m_ica_compoments_offset 	= 0;

	image_w                     = 0;
	image_h                     = 0;
	m_image_matrix              = NULL;

	m_covar 					=
	m_evec 						=
	m_projection 				=
	m_reconstruction 			=
	m_residual 					=
	m_residual_covar 			=
	m_residual_evec 			=
	m_residual_projection 		=
	m_residual_reconstruction 	=
	m_residual_residual 		= NULL;

	m_mean						=
	m_eval						=
	m_residual_mean				=
	m_residual_eval				= NULL;

    pthread_cond_init(&m_Cond, NULL);
    pthread_mutex_init(&m_Mutex, NULL);

    // Detection
    m_one_step_mode             = false;
    m_detectionready            = false;
    m_detectionscore            = 0.0;
    m_detectiondone             = true;

    m_imgy                      = NULL;
}

CLocalFeature::~CLocalFeature()
{
//	printf("[%s]\n", __func__);

	if (m_vm_name)
		free(m_vm_name);

	if (m_local_name)
		free(m_local_name);

	m_local_type = FCWS__LOCAL__TYPE__UnKonwn;
	m_para_count = 0;

	for (int i=0 ; i<FCWS__PARA__TYPE__TOTAL ; i++)
	{
		if (m_para[i])
		{
			delete m_para[i];
			m_para[i] = NULL;
		}
	}

    if (m_para2)
    {
        delete m_para2;
        m_para2 = NULL;
    }

	if (m_image_matrix)
		gsl_matrix_free(m_image_matrix);

	if (m_mean)
		gsl_vector_free(m_mean);

	if (m_covar)
		gsl_matrix_free(m_covar);

	if (m_eval)
		gsl_vector_free(m_eval);

	if (m_evec)
		gsl_matrix_free(m_evec);

	if (m_projection)
		gsl_matrix_free(m_projection);

	if (m_reconstruction)
		gsl_matrix_free(m_reconstruction);

	if (m_residual)
		gsl_matrix_free(m_residual);

	if (m_residual_mean)
		gsl_vector_free(m_residual_mean);

	if (m_residual_covar)
		gsl_matrix_free(m_residual_covar);

	if (m_residual_eval)
		gsl_vector_free(m_residual_eval);

	if (m_residual_evec)
		gsl_matrix_free(m_residual_evec);

	if (m_residual_projection)
		gsl_matrix_free(m_residual_projection);

	if (m_residual_reconstruction)
		gsl_matrix_free(m_residual_reconstruction);

	if (m_residual_residual)
		gsl_matrix_free(m_residual_residual);

    // Detection
    if (m_imgy)
        gsl_matrix_free(m_imgy);



}

bool CLocalFeature::HasParam()
{
    return m_has_param;
}

int CLocalFeature::SetParam(FCWS__Local__Type local_type, FCWS__Para__Type para_type, gsl_matrix *from)
{
	assert(from != NULL);

	m_local_type = local_type;

	if (m_para[para_type])
	{
		delete m_para[para_type];
		m_para_count--;
	}

	m_para[para_type] = new CParam(para_type);

	m_para[para_type]->SetParam(para_type, from);

	m_para_count++;

	return 0;
}

int CLocalFeature::LoadParam(FCWS__Local__Type local_type, FCWS__Para__Type para_type, int rows, int cols, double *from)
{
	assert(from != NULL);

	m_local_type = local_type;

	if (m_para[para_type])
	{
		delete m_para[para_type];
		m_para_count--;
	}

	m_para[para_type] = new CParam(para_type);

	m_para[para_type]->LoadParam(para_type, rows, cols, from);

	m_para_count++;

	return 0;
}

bool CLocalFeature::LoadParam(FCWS__Local__Type local_type, FCWS__Para2* param)
{
    if (!param)
        return false;

	m_local_type = local_type;

    if (m_para2)
    {
        delete m_para2;
        m_para2 = NULL;
    }

    m_para2 = new CParam2();
    m_has_param = m_para2->LoadParam(param);
    
    return m_has_param;
}

int CLocalFeature::SetParam(FCWS__Para__Type para_type, gsl_matrix *from)
{
	assert(from != NULL);

	if (m_para[para_type])
	{
		delete m_para[para_type];
		m_para[para_type] = NULL;
		m_para_count--;
	}

	m_para[para_type] = new CParam(para_type);
	m_para[para_type]->SetParam(para_type, from);

	m_para_count++;

	return 0;
}

int CLocalFeature::LoadParam(FCWS__Para__Type para_type, int rows, int cols, double *from)
{
	assert(from != NULL);

	if (m_para[para_type])
	{
		delete m_para[para_type];
		m_para[para_type] = NULL;
		m_para_count--;
	}

	m_para[para_type] = new CParam(para_type);
	m_para[para_type]->LoadParam(para_type, rows, cols, from);

	m_para_count++;

	return 0;
}

bool CLocalFeature::LoadParam(FCWS__Para2* param)
{

    if (!param)
        return false;

    if (m_para2)
    {
        delete m_para2;
        m_para2 = NULL;
    }

    m_para2 = new CParam2();
    m_has_param = m_para2->LoadParam(param);

    return m_has_param;
}

bool CLocalFeature::SaveParam(FCWS__Para__Type para_type, FCWS__Para *param)
{
	if (m_para[para_type])
		return m_para[para_type]->SaveParam(param);

	return false;
}

bool CLocalFeature::SaveParam(FCWS__Para2 *param)
{
    if (m_para2)
        return m_para2->SaveParam(param);

    return false;
}

void CLocalFeature::SetPCAAndICAComponents(int pca_first_k_components, int pca_compoments_offset, int ica_first_k_components, int ica_compoments_offset)
{
	m_pca_first_k_components	= pca_first_k_components;
	m_pca_compoments_offset 	= pca_compoments_offset;
	m_ica_first_k_components 	= ica_first_k_components;
	m_ica_compoments_offset 	= ica_compoments_offset;
}

void CLocalFeature::SetOutputPath(string output_folder)
{
	m_output_folder = output_folder;
}

int CLocalFeature::PickUpFiles(vector<string> & feedin, int rows, int cols)
{
	char temp[MAX_FN_L];
	char *pch = NULL;
	vector<string>::iterator it;
	int w = 0, h = 0;


	if (feedin.size() > 0)
	{
//		image_w = rows;
//		image_h = cols;

		m_filelist.clear();

		printf("->Search local image: %s\n", m_local_name);

//		printf("%d %d\n", m_filelist.size(), feedin.size());

		for (it = feedin.begin(); it != feedin.end() ; it++)
		{
//			printf("%s\n", it->c_str());
			if (strlen((char*)it->c_str()))
			{
				pch = strstr((char*)it->c_str(), search_local_model_pattern[(int)m_local_type]);
				if (pch)
				{
					// format: XXX_Right_220_30.yuv
					sscanf(pch, "%*[^_]_%d_%d", &w, &h);
					printf("%s\n", it->c_str());

					if (image_w == 0 && image_h == 0)
					{
						image_w = w;
						image_h = h;
					}

					if (image_w && image_w == w && image_h && image_h == h)
						m_filelist.push_back(it->c_str());

					it->clear();
				}
			}
		}

//		printf("%d %d\n\n", m_filelist.size(), feedin.size());

		sort(m_filelist.begin(), m_filelist.end());

		GenImageMat();
	}


	return 0;
}





//Para* LocalModel::GetParaObj(FCWS__Para__Type para_type)
//{
//	if (m_para[para_type])
//		return m_para[para_type]->GetObj();
//
//	return NULL;
//}

FCWS__Local__Type CLocalFeature::GetLocalType()
{
	return m_local_type;
}

void CLocalFeature::DeleteParaObj(FCWS__Para__Type para_type)
{
	if (m_para[para_type])
	{
		delete m_para[para_type];
		m_para[para_type] = NULL;
	}
}

int CLocalFeature::DoTraining()
{
	int ret = 0, rows, cols;

	if (m_image_matrix)
	{
		// PCA of original images.
		ret = PCA(m_image_matrix,
				  &m_mean,
				  &m_covar,
				  &m_eval,
				  &m_evec,
				  m_pca_first_k_components,
				  m_pca_compoments_offset,
                  &m_FirstKEval,
				  &m_FirstKEigVector,
				  &m_projection,
				  &m_reconstruction,
				  &m_residual);

		printf("1st-order PCA of %s of %s is done.\n",
				search_local_model_pattern[m_local_type],
				search_vehicle_model_pattern[m_vm_type]);

		//WriteBackMatrixToImages(m_reconstruction, "recontruct");
//		SetParam(FCWS__PARA__TYPE__PCA, &m_FirstKEigVector.matrix);

		if (ret == GSL_SUCCESS)
		{
            SetPCAParam(m_mean, &m_FirstKEval.vector, &m_FirstKEigVector.matrix);

			// PCA of residual images.
			ret = PCA(m_residual,
				  &m_residual_mean,
				  &m_residual_covar,
				  &m_residual_eval,
				  &m_residual_evec,
				  m_pca_first_k_components,
				  m_pca_compoments_offset,
                  &m_residual_FirstKEval,
				  &m_residual_FirstKEigVector,
				  &m_residual_projection,
				  &m_residual_reconstruction,
				  &m_residual_residual
				);

			//WriteBackMatrixToImages(m_residual_reconstruction, "residual_recontruct");
			//SetParam(FCWS__PARA__TYPE__PCA2, &m_residual_FirstKEigVector.matrix);

			if (ret == GSL_SUCCESS)
			{
                SetPCA2Param(m_residual_mean, &m_residual_FirstKEval.vector, &m_residual_FirstKEigVector.matrix);
				printf("2nd-order PCA of %s of %s is done.\n",
						search_local_model_pattern[m_local_type],
						search_vehicle_model_pattern[m_vm_type]);

//				int compc = 2;
				gsl_matrix *K = NULL, *W = NULL, *A = NULL, *S = NULL;
//
//				rows = m_residual_FirstKEigVector.matrix.size1;
//				cols = m_residual_FirstKEigVector.matrix.size2;
//
//				K = gsl_matrix_alloc(cols, compc);
//				W = gsl_matrix_alloc(compc, compc);
//				A = gsl_matrix_alloc(compc, compc);
//				S = gsl_matrix_alloc(rows, cols);
//
//				ret = FastICA(&m_residual_FirstKEigVector.matrix, compc, K, W, A, S);
//
//				printf("FastICA of %s of %s is done.\n",
//						search_local_model_pattern[m_local_type],
//						search_vehicle_model_pattern[m_vm_type]);
//
//				SetParam(FCWS__PARA__TYPE__ICA, S);

//				WriteBackMatrixToImages(m_reconstruction, "recontruct");
//				WriteBackMatrixToImages(m_residual_reconstruction, "residual_recontruct");
//				WriteBackMatrixToImages(S, "fastICA");

				if (K)
				{
					gsl_matrix_free(K);
					K = NULL;
				}

				if (W)
				{
					gsl_matrix_free(W);
					W = NULL;
				}

				if (A)
				{
					gsl_matrix_free(A);
					A = NULL;
				}

				if (S)
				{
					gsl_matrix_free(S);
					S = NULL;
				}

			}
		}
		else
		{
			ret = -1;
		}
	}

	// printf("[LocalModel::DoTraining %d] Done\n", m_local_type);

	return ret;
}

bool CLocalFeature::DetectionReady()
{
    return m_detectionready;
}

bool CLocalFeature::IsIdle()
{
    return m_detectiondone;
}

double CLocalFeature::GetScore()
{
    double score = 0.0;

    pthread_mutex_lock(&m_Mutex);
    score = m_detectionscore;
    pthread_mutex_unlock(&m_Mutex);
}

int CLocalFeature::DoDetection()
{
    int rows, cols;
    int temp_r, temp_c;
    int sw_r, sw_c, sw_w, sw_h;
    gsl_vector* img_vector = NULL;

    m_detectionready = true;

    while (!m_terminate)
    {
        pthread_mutex_lock(&m_Mutex);
        pthread_cond_wait(&m_Cond, &m_Mutex);

        if (m_terminate) {
            pthread_mutex_unlock(&m_Mutex);
            break;
        }

        if (m_imgy)
        {
            rows = m_imgy->size1;
            cols = m_imgy->size2;

            m_sw.GetWH(sw_w, sw_h);

            if(!img_vector || img_vector->size != (sw_w * sw_h))
            {
                if (img_vector)
                    gsl_vector_free(img_vector);

                if ((img_vector = gsl_vector_alloc(sw_w * sw_h)) == NULL)
                    continue;
            }

            gsl_vector_set_zero(img_vector);

            // TODO
            // Convert matrix to vector









        }

        // shift window
        m_sw.GetPos(sw_r, sw_c);
        m_sw.GetWH(sw_w, sw_h);

        if (1 || m_local_type == FCWS__LOCAL__TYPE__LEFT)
        {
            printf("[%s][%d] lf %s of %s: (%d,%d) (%d,%d) (%d, %d)\n", 
                    __func__, __LINE__, 
                    search_local_model_pattern[m_local_type],
                    search_vehicle_model_pattern[m_vm_type],
                    sw_r, sw_c,
                    sw_w, sw_h,
                    rows, cols);

            if (m_one_step_mode)
            {
                bool success = false;

                temp_c = sw_c;
                temp_r = sw_r;

                if ((temp_c + 1) < (cols - sw_w))
                {
                    temp_c++;
                    success = true;
                }
                else
                {
                    temp_c = 0;
                    if ((temp_r + 1) < (rows - sw_h))
                    {
                        temp_r++;
                        success = true;
                    }
                }

                if (success)
                    m_sw.SetPos(temp_r, temp_c);
            }
            else
            {
                for (temp_r = sw_r ; temp_r < (rows - sw_h) ; temp_r++)
                {
                    for (temp_c = sw_c ; temp_c < (cols - sw_w) ; temp_c++)
                    {
                        //printf("[%d][%d] (%d, %d)\n", temp_r, temp_c, rows - sw_h, cols - sw_w);
                        m_para2->CalProbability();
                    }
                } 
            }
        }

        m_detectiondone = true;
        pthread_mutex_unlock(&m_Mutex);

    }

    m_detectiondone = true;
    m_detectionready = false;

    return 0;
}

int CLocalFeature::TriggerDetection(bool onestep)
{
    pthread_mutex_lock(&m_Mutex);

    m_one_step_mode = onestep;
    m_detectiondone = false;
    m_detectionscore = .0;

    pthread_cond_signal(&m_Cond);
    pthread_mutex_unlock(&m_Mutex);

    return 0;
}

void CLocalFeature::Stop()
{
    pthread_mutex_lock(&m_Mutex);
    pthread_cond_signal(&m_Cond);
    pthread_mutex_unlock(&m_Mutex);

    m_terminate = true;
}

void CLocalFeature::SetLocalImg(uint8_t *imgy, int o_w, int o_h, int r, int c, int w, int h, int pitch)
{
    int rr, cc;
    int shift = (r * o_w) + c;

    if (!imgy || !w || !h)
        return;

    // In the following matrix, size1 is 3 and size2 is 4.
    // 00 01 02 03 XX XX XX XX
    // 10 11 12 13 XX XX XX XX
    // 20 21 22 23 XX XX XX XX
    //
    if(!m_imgy || m_imgy->size1 != h || m_imgy->size2 != w)
    {
        if (m_imgy)
            gsl_matrix_free(m_imgy);

        if ((m_imgy = gsl_matrix_alloc(h, w)) == NULL)
            return;
    }

    gsl_matrix_set_zero(m_imgy);

    for (rr = 0 ; rr < m_imgy->size1 ; rr++)
        for (cc = 0 ; cc < m_imgy->size2 ; cc++)
            gsl_matrix_set(m_imgy, rr, cc, imgy[ shift + rr * pitch + cc]);

    switch (m_local_type)
    {
        default:
        case FCWS__LOCAL__TYPE__LEFT:
        case FCWS__LOCAL__TYPE__RIGHT:
            m_sw.SetInfo(0, 0, 30, 50);
            break;
        case FCWS__LOCAL__TYPE__CENTER: 
            m_sw.SetInfo(0, 0, 75, 20);
            break;
    }
#if 0
    static int count = 0;
    char fout[512] = {"\0"};
    FILE *fd;
    uint8_t *temp;

    temp = (uint8_t*)malloc(sizeof(uint8_t) * (((h*w)*3) >> 1));

    if (temp)
    {
        memset(temp, 128, (((h*w)*3) >> 1));
        snprintf(fout, 512, "%03d_%d_%d.yuv", ++count, w, h);
        printf("fout %s\n", fout);

        if ((fd = fopen(fout, "w")) != NULL)
        {
            for (rr = 0 ; rr < m_imgy->size1 ; rr++)
                for (cc = 0 ; cc < m_imgy->size2 ; cc++)
                    temp[rr * m_imgy->size2 + cc] = (uint8_t)gsl_matrix_get(m_imgy, rr, cc);

            fwrite(temp, 1, (((h*w)*3) >> 1), fd);
            fclose(fd);
        }
        free(temp);
    }
#endif
}

void CLocalFeature::GetSWInfo(CShiftWindow &sw)
{
    sw = m_sw;
}














//-------------------------------------------------------------------
bool CLocalFeature::SetPCAParam(gsl_vector *mean, gsl_vector *eval, gsl_matrix *evec)
{
    if (!m_para2 || !mean || !eval || !evec)
        return false;

    return m_para2->SetPCAParam(mean, eval, evec);
}

bool CLocalFeature::SetPCA2Param(gsl_vector *mean, gsl_vector *eval, gsl_matrix *evec)
{
    if (!m_para2 || !mean || !eval || !evec)
        return false;
    
    return m_para2->SetPCA2Param(mean, eval, evec);
}

int	CLocalFeature::GenImageMat()
{
	int i, j, y_size, file_count = 0;
	unsigned char *temp = NULL, pixel_data;
	gsl_vector_uchar_view image_y;
	gsl_vector_view image_matrix_column_view;
	FILE *fd = NULL;

	file_count = m_filelist.size();

	if (file_count && image_w && image_h)
	{
		printf("[%s] file_count %d(%dx%d) of %s of %s\n\n", __func__, file_count, image_w, image_h, m_local_name, m_vm_name);

		y_size = image_w * image_h;
		temp = (unsigned char*)malloc(sizeof(unsigned char) * y_size);
		m_image_matrix = gsl_matrix_alloc(y_size, file_count);

		if (m_image_matrix)
		{
			for (i=0 ; i<file_count ; i++)
			{
				if ((fd = fopen(m_filelist[i].c_str(), "r")) != NULL)
				{
//					printf("open %s\n", m_filelist[i].c_str());
					fread(temp, 1, y_size, fd);
					image_y = gsl_vector_uchar_view_array(temp, y_size);
					image_matrix_column_view = gsl_matrix_column(m_image_matrix, i);

					for (j=0 ; j<image_matrix_column_view.vector.size ; j++)
					{
		    			pixel_data = gsl_vector_uchar_get(&image_y.vector, j);
		    			gsl_vector_set(&image_matrix_column_view.vector, j, pixel_data);
					}

					fclose(fd);
					fd = NULL;
				}
				else
					printf("Can not open %s\n", m_filelist[i].c_str());
			}
		}
		else
		{
			return -1;
		}
	}

	return 0;
}

gsl_vector*	CLocalFeature::CalMean(gsl_matrix *m)
{
	assert(m != NULL);

	int i, j;
	int rows, cols;
	gsl_vector *mean = NULL;
	float value;
	// unsigned long long start_time, end_time;

	// start_time = get_time();

	rows = m->size1;
	cols = m->size2;
//	printf("rows:%d, cols:%d\n", rows, cols);

	mean = gsl_vector_alloc(rows);
	gsl_vector_set_zero(mean);

	for (i=0 ; i<rows ; i++)
	{
		value = gsl_stats_mean(m->data + i * cols, 1, cols);
//		printf("mean[%d]=%f\n", i, value);
		gsl_vector_set(mean, i, value);
	}

	// end_time = get_time();
	// printf("%s of %s of %s: %llums\n", __func__, m_local_name, m_vm_name, end_time - start_time);

	return mean;
}

gsl_matrix* CLocalFeature::CalVariance(gsl_matrix *m, gsl_vector* mean)
{
	assert(m != NULL);
	assert(mean != NULL);

	int i;
	int rows, cols;
	// unsigned long long start_time, end_time;

	// start_time = get_time();

	rows = m->size1;
	cols = m->size2;

//	printf("%s: rows:%d, cols: %d\n", __func__, rows, cols);
	gsl_matrix *mean_subtracted_m, *covariance_matrix;
	gsl_vector_view mean_subtracted_m_cloumn_view;

	mean_subtracted_m = gsl_matrix_alloc(rows, cols);
	gsl_matrix_memcpy(mean_subtracted_m, m);

	for (i=0 ; i<cols ; i++)
	{
		mean_subtracted_m_cloumn_view = gsl_matrix_column(mean_subtracted_m, i);
		gsl_vector_sub(&mean_subtracted_m_cloumn_view.vector, mean);
	}

	covariance_matrix = gsl_matrix_alloc(rows, rows);
	gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0 / (double)(cols - 1), mean_subtracted_m, mean_subtracted_m, 0.0, covariance_matrix);

	gsl_matrix_free(mean_subtracted_m);

//	double max, min;
//	gsl_matrix_minmax(covariance_matrix, &min, &max);
//	printf("%s: max: %f, min: %f\n", __func__, max, min);

	// end_time = get_time();
	// printf("%s of %s of %s: %llums\n", __func__, m_local_name, m_vm_name, end_time - start_time);

	return covariance_matrix;
}


int CLocalFeature::CalEigenSpace(gsl_matrix* covar, gsl_vector** eval, gsl_matrix** evec)
{
	assert(covar != NULL);

	int i, rows, cols, ret = 0;
	gsl_vector *pEValue;
	gsl_matrix *pEVctor;
    gsl_eigen_symmv_workspace* workspace;

//	unsigned long long start_time, end_time;

//	start_time = GetTime();

	rows = covar->size1;
	cols = covar->size2;
//	printf("%s: rows:%d, cols: %d\n", __func__, rows, cols);

	pEValue = gsl_vector_alloc(rows);
	gsl_vector_set_zero(pEValue);

	pEVctor = gsl_matrix_alloc(rows, rows);
	gsl_matrix_set_zero(pEVctor);

	workspace = gsl_eigen_symmv_alloc(rows);
	ret = gsl_eigen_symmv(covar, pEValue, pEVctor, workspace);
    gsl_eigen_symmv_free(workspace);

    if (ret == GSL_SUCCESS)
    {
    	gsl_eigen_symmv_sort(pEValue, pEVctor, GSL_EIGEN_SORT_VAL_DESC);

		*eval = pEValue;
		*evec = pEVctor;
    }
    else
    {
    	gsl_vector_free(pEValue);
    	gsl_matrix_free(pEVctor);
    }

//	end_time = GetTime();
//	printf("%s of %s of %s: %llums\n", __func__, m_local_name, m_vm_name, end_time - start_time);

	return ret;
}

gsl_vector_view CLocalFeature::GetFirstKComponents(gsl_vector *eval, int firstk)
{
    return gsl_vector_subvector(eval, 0, firstk);
}

gsl_matrix_view CLocalFeature::GetFirstKComponents(gsl_matrix *evec, int firstk)
{
	return gsl_matrix_submatrix(evec, 0, 0, evec->size1, firstk);
}

/*
* R = P-Transpose * X, where R is KxM, P is NxK, X is NxM
*/
gsl_matrix* CLocalFeature::GenerateProjectionMatrix(gsl_matrix *p, gsl_matrix *x)
{
	assert(p != NULL);
	assert(x != NULL);

	int rows, cols;
	gsl_matrix *r = NULL;

	rows = p->size2;	// K
	cols = x->size2;	// M

	r = gsl_matrix_alloc(rows, cols);
	gsl_matrix_set_zero(r);

	gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, p, x, 0.0, r);

	return r;
}

// X` = P X R, where X` is NxM, P is NxK, R is KxM
gsl_matrix* CLocalFeature::GenerateReconstructImageMatrixFromPCA(gsl_matrix *p, gsl_matrix *r, gsl_vector *mean)
{
	assert(p != NULL);
	assert(r != NULL);
	assert(mean != NULL);

	int i, j;
	int rows, cols;
	unsigned char pix_datat;
	double max, min, temp_data;
	gsl_matrix *x = NULL;
	gsl_vector_view x_col_view;
//	gsl_matrix_uchar *x = NULL;

	rows = p->size1;
	cols = r->size2;

	x = gsl_matrix_alloc(rows, cols);
	gsl_matrix_set_zero(x);

//	x = gsl_matrix_uchar_alloc(rows, cols);
//	gsl_matrix_uchar_set_zero(x);

	// X`= P X R.
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, p, r, 0.0, x);

	// Add mean
	for (i=0 ; i<cols ; i++)
	{
		x_col_view = gsl_matrix_column(x, i);
		gsl_vector_add(&x_col_view.vector, mean);
	}

	// Get max and min value for normalization.
	gsl_matrix_minmax(x, &min, &max);
//	printf("min %lf, max %lf\n", min, max);

	for (i=0 ; i<rows ; i++)
	{
		for (j=0 ; j<cols ; j++)
		{
			temp_data = gsl_matrix_get(x, i, j);
			temp_data = ((temp_data - min) / (max - min)) * 255;
			gsl_matrix_set(x, i, j, temp_data);
		}
	}

	gsl_matrix_minmax(x, &min, &max);
//	printf("min %lf, max %lf\n", min, max);

//	for (i=0 ; i<rows ; i++)
//	{
//		for (j=0 ; j<cols ; j++)
//		{
//			pix_datat = (unsigned char)gsl_matrix_get(x_temp, i, j);
//			gsl_matrix_uchar_set(x, i, j, pix_datat);
//		}
//	}
//
//	if (x_temp)
//		gsl_matrix_free(x_temp);

	return x;
}

gsl_matrix* CLocalFeature::GenerateResidualImageMatrix(gsl_matrix *orignal_image_matrix, gsl_matrix *reconstruct_image_matrix)
{
	assert(orignal_image_matrix != NULL);
	assert(reconstruct_image_matrix != NULL);
	assert(orignal_image_matrix->size1 == reconstruct_image_matrix->size1);
	assert(orignal_image_matrix->size2 == reconstruct_image_matrix->size2);

	int i, j, rows, cols;
	double min, max, pixel_data;

	gsl_matrix *output_im = NULL;

	rows = orignal_image_matrix->size1;
	cols = orignal_image_matrix->size2;

	output_im = gsl_matrix_alloc(rows, cols);
	gsl_matrix_memcpy(output_im, orignal_image_matrix);

	// Residual = Orignal - Reconstruct.
	gsl_matrix_sub(output_im , reconstruct_image_matrix);

	// Get max and min value for normalization.
	gsl_matrix_minmax(output_im, &min, &max);

	for (i=0 ; i<rows ; i++)
	{
		for (j=0 ; j<cols ; j++)
		{
			pixel_data = gsl_matrix_get(output_im, i, j);
			pixel_data = ((pixel_data - min) / (max - min)) * 255;
			gsl_matrix_set(output_im, i, j, pixel_data);
		}
	}

	return output_im;
}

int CLocalFeature::PCA
(
	gsl_matrix *im,
	gsl_vector **mean,
	gsl_matrix **covar,
	gsl_vector **eval,
	gsl_matrix **evec,
	int first_k_comp,
	int comp_offset,
    gsl_vector_view *principle_eval,
	gsl_matrix_view *principle_evec,
	gsl_matrix **projection,
	gsl_matrix **reconstruction_images,
	gsl_matrix **residual_images
)
{
	assert(im != NULL);

	int ret = 0;

	*mean 	= CalMean(im);
	*covar 	= CalVariance(im, *mean);
	ret 	= CalEigenSpace(*covar, eval, evec);

	if (ret == GSL_SUCCESS)
	{
        *principle_eval         = GetFirstKComponents(*eval, first_k_comp);
		*principle_evec 		= GetFirstKComponents(*evec, first_k_comp);
		*projection 			= GenerateProjectionMatrix(&(principle_evec->matrix), im);
		*reconstruction_images 	= GenerateReconstructImageMatrixFromPCA(&(principle_evec->matrix), *projection, *mean);
		*residual_images 		= GenerateResidualImageMatrix(im, *reconstruction_images);
	}
	else
	{
		ret = -1;
	}

	return ret;
}

/* ICA related function */

/**
 * Centers mat M. (Subtracts the mean from every column)
 */
void CLocalFeature::MatrixCenter(gsl_matrix *m, gsl_vector **mean)
{
	assert(m != NULL);

	int i, j;
	int rows, cols;
	gsl_vector *pmean = NULL;
	float value;
	// unsigned long long start_time, end_time;
	gsl_vector_view mean_subtracted_m_cloumn_view;

	// start_time = get_time();

	rows = m->size1;
	cols = m->size2;
	// printf("rows:%d, cols:%d\n", rows, cols);

	pmean = gsl_vector_alloc(cols);
	gsl_vector_set_zero(pmean);

	for (i=0 ; i<cols ; i++)
	{
		value = gsl_stats_mean(m->data + i, cols, rows);
		// printf("mean[%d]=%f\n", i, value);
		gsl_vector_set(pmean, i, value);
	}


	for (i=0 ; i<cols ; i++)
	{
		mean_subtracted_m_cloumn_view = gsl_matrix_column(m, i);
		gsl_vector_add_constant(&mean_subtracted_m_cloumn_view.vector, - gsl_vector_get(pmean, i));
	}

	*mean = pmean;
	// end_time = get_time();
	// printf("%s of %s of %s: %llums\n", __func__, m_local_name, m_vm_name, end_time - start_time);
}

void CLocalFeature::MatrixDeCenter(gsl_matrix *m, gsl_vector *mean)
{
	assert(m != NULL);
	assert(mean != NULL);

	int i, j;

	for (i=0 ; i<m->size1 ; i++)
		for(j=0 ; j<m->size2 ; j++)
			gsl_matrix_set(m, i, j, gsl_matrix_get(m, i, j) + gsl_vector_get(mean, j));
}

gsl_matrix* CLocalFeature::MatrixInvert(gsl_matrix *m)
{
	assert(m != NULL);

	int s, rows, cols;
	gsl_matrix *inv = NULL, *tempm = NULL;
	gsl_permutation *p;

	rows = m->size1;
	cols = m->size2;

	if (rows != cols)
	{
		printf("[MatrixInvert] Source matrix is not a square matrix.\n");
		return NULL;
	}

	tempm = gsl_matrix_alloc(rows, cols);
	inv   = gsl_matrix_alloc(rows, cols);
	p 	  = gsl_permutation_alloc(rows);

	gsl_matrix_memcpy(tempm, m);
	gsl_linalg_LU_decomp(tempm, p, &s);
	gsl_linalg_LU_invert(tempm, p, inv);

	if (tempm)
		gsl_matrix_free(tempm);

	if (p)
		gsl_permutation_free(p);

	return inv;
}

gsl_matrix* CLocalFeature::ICA_compute(gsl_matrix *X)
{
	assert(X != NULL);

	gsl_matrix *TXp, *GWX, *W, *Wd, *W1, *D, *TU, *TMP, *SVD_WorkMat;
	gsl_vector *d, *lim, *SVD_WorkVect;
	int i, j, it, rows, cols;
	double max, val;

	rows = X->size1;
	cols = X->size2;

	TXp = gsl_matrix_alloc(cols, rows);
	GWX = gsl_matrix_alloc(rows, cols);
	W 	= gsl_matrix_alloc(rows, rows);
	Wd 	= gsl_matrix_alloc(rows, rows);
	D 	= gsl_matrix_alloc(rows, rows);
	TMP = gsl_matrix_alloc(rows, rows);
	TU 	= gsl_matrix_alloc(rows, rows);
	W1  = gsl_matrix_alloc(rows, rows);
	d 	= gsl_vector_alloc(rows);

	// W rand init
	srand(time(NULL));

	for (i=0 ; i<W->size1 ; i++)
		for (j=0 ; j<W->size2 ; j++)
			gsl_matrix_set(W, i, j, rand()/(double)RAND_MAX);

	// sW <- La.svd(W)
	gsl_matrix_memcpy(Wd, W);
	// gsl_linalg_SV_decomp_jacobi(Wd, D, d);	//?????

	SVD_WorkMat = gsl_matrix_alloc(Wd->size2, Wd->size2);
	SVD_WorkVect = gsl_vector_alloc(Wd->size2);
	// printf("%s %d: %dx%d, %d\n", __func__, __LINE__, SVD_WorkMat->size1, SVD_WorkMat->size2, SVD_WorkVect->size);
	gsl_linalg_SV_decomp_mod(Wd, SVD_WorkMat, D, d, SVD_WorkVect);

	if (SVD_WorkMat)
	{
		gsl_matrix_free(SVD_WorkMat);
		SVD_WorkMat = NULL;
	}

	if (SVD_WorkVect)
	{
		gsl_vector_free(SVD_WorkVect);
		SVD_WorkVect = NULL;
	}

	// W <- sW$u %*% diag(1/sW$d) %*% t(sW$u) %*% W
	gsl_matrix_transpose_memcpy(TU, Wd);

	for (i=0 ; i<d->size ; i++)
		gsl_vector_set(d, i, 1 / gsl_vector_get(d, i));

	gsl_matrix_set_zero(D);

	for (i=0 ; i<d->size ; i++)
		gsl_matrix_set(D, i, i, gsl_vector_get(d, i));

	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, Wd, D, 0.0, TMP);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, TMP, TU, 0.0, D);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, D, W, 0.0, Wd);

	// W1 <- W
	gsl_matrix_memcpy(W1, Wd);

	// lim <- rep(1000, maxit); it = 1
	lim = gsl_vector_alloc(MAX_ITERATIONS+1);	// avoid bounary exception.
	gsl_vector_set_zero(lim);
	gsl_vector_add_constant(lim, 1000.0);
	it = 0;

	// t(X)/p
	gsl_matrix_transpose_memcpy(TXp, X);
	gsl_matrix_scale(TXp, 1 / (double)TXp->size1);

	while (gsl_vector_get(lim, it) > TOLERANCE && it < MAX_ITERATIONS)
	{
		// wx <- W %*% X
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, Wd, X, 0.0, GWX);

		// gwx <- tanh(alpha * wx)
		for (i=0 ; i<GWX->size1 ; i++)
			for (j=0 ; j<GWX->size2 ; j++)
				gsl_matrix_set(GWX, i, j, tanh(ALPHA * gsl_matrix_get(GWX, i, j)));

		// v1 <- gwx %*% t(X)/p
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, GWX, TXp, 0.0, TMP);

		// g.wx <- alpha * (1 - (gwx)^2)
		for (i=0 ; i<GWX->size1 ; i++)
			for (j=0 ; j<GWX->size2 ; j++)
				gsl_matrix_set(GWX, i, j, ALPHA * ( 1 - gsl_pow_2(gsl_matrix_get(GWX, i, j))));

		// v2 <- diag(apply(g.wx, 1, FUN = mean)) %*% W
		gsl_vector_set_zero(d);
		d = CalMean(GWX);

		gsl_matrix_set_zero(D);
		for (i=0 ; i<d->size ; i++)
			gsl_matrix_set(D, i, i, gsl_vector_get(d, i));

		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, D, Wd, 0.0, TU);

		// W1 <- v1 - v2
		gsl_matrix_memcpy(W1, TMP);
		gsl_matrix_sub(W1, TU);

		// sW1 <- La.svd(W1)
		gsl_matrix_memcpy(W, W1);
		// gsl_linalg_SV_decomp_jacobi(W, D, d);	//?????

		if (SVD_WorkMat == NULL)
			SVD_WorkMat = gsl_matrix_alloc(W->size2, W->size2);

		if (SVD_WorkVect == NULL)
			SVD_WorkVect = gsl_vector_alloc(W->size2);
		gsl_linalg_SV_decomp_mod(W, SVD_WorkMat, D, d, SVD_WorkVect);

		// W1 <- sW1$u %*% diag(1/sW1$d) %*% t(sW1$u) %*% W1
		gsl_matrix_transpose_memcpy(TU, W);
		for (i=0 ; i<d->size ; i++)
			gsl_vector_set(d, i, 1 / gsl_vector_get(d, i));

		gsl_matrix_set_zero(D);
		for (i=0 ; i<d->size ; i++)
			gsl_matrix_set(D, i, i, gsl_vector_get(d, i));

		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, W, D, 0.0, TMP);
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, TMP, TU, 0.0, D);
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, D, W1, 0.0, W);

		// lim[it + 1] <- max(Mod(Mod(diag(W1 %*% t(W))) - 1))
		gsl_matrix_transpose_memcpy(TU, Wd);
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, W, TU, 0.0, TMP);

		gsl_vector_view diagonal = gsl_matrix_diagonal(TMP);
		max = gsl_vector_get(&diagonal.vector, 0);
		for (i=1 ; i<diagonal.vector.size ; i++)
		{
			val = gsl_vector_get(&diagonal.vector, i);
			if (val > max)
				max = val;
		}

		gsl_vector_set(lim, it+1, fabs(max - 1));

		// W <- W1
		gsl_matrix_memcpy(Wd, W);

		it++;
	}

	if (TXp)
		gsl_matrix_free(TXp);

	if (GWX)
		gsl_matrix_free(GWX);

	if (W)
		gsl_matrix_free(W);

	if (D)
		gsl_matrix_free(D);

	if (TMP)
		gsl_matrix_free(TMP);

	if (TU)
		gsl_matrix_free(TU);

	if (W1)
		gsl_matrix_free(W1);

	if (d)
		gsl_vector_free(d);

	if (SVD_WorkMat)
		gsl_matrix_free(SVD_WorkMat);

	if (SVD_WorkVect)
		gsl_vector_free(SVD_WorkVect);

	return Wd;
}

/**
 * Main FastICA function. Centers and whitens the input
 * matrix, calls the ICA computation function ICA_compute()
 * and computes the output matrixes.
 *
 * http://tumic.wz.cz/fel/online/libICA/
 *
 * X: 		pre-processed data matrix [rows, cols]
 * compc: 	number of components to be extracted
 * K: 		pre-whitening matrix that projects data onto the first compc principal components
 * W:		estimated un-mixing matrix
 * A: 		estimated mixing matrix
 * S:		estimated source matrix
 *
 * Ref: 	https://en.wikipedia.org/wiki/FastICA
 *
 */
int CLocalFeature::FastICA
(
	gsl_matrix *X,
	int compc,
	gsl_matrix *K,
	gsl_matrix *W,
	gsl_matrix *A,
	gsl_matrix *S
)
{
	int i, j;
	int ret= 0, rows, cols;
	gsl_matrix *XT, *V, *TU, *D, *X1, *_A, *InvMat;
	gsl_vector *scale, *d, *mean;
	gsl_matrix_view subMat1, subMat2, subMat3;

	rows = X->size1;
	cols = X->size2;

	XT = gsl_matrix_alloc(cols, rows);
	X1 = gsl_matrix_alloc(compc, rows);
	V  = gsl_matrix_alloc(cols, cols);
	D  = gsl_matrix_alloc(cols, cols);
	TU = gsl_matrix_alloc(cols, cols);
	scale = gsl_vector_alloc(cols);
	d  = gsl_vector_alloc(cols);

	/*
	 * CENTERING
	 */
	MatrixCenter(X, &mean);

	/*
	 * WHITENING
	 */
	gsl_matrix_transpose_memcpy(XT, X);
	// X[i][j] = X[i][j] * (1/rows)
	gsl_matrix_scale(X, 1 / (double)rows);

	gsl_matrix_set_zero(V);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, XT, X, 0.0, V); // V is right

	// SVD on V
	// refer to  https://www.gnu.org/software/gsl/doc/html/linalg.html#singular-value-decomposition
#if 0
	gsl_matrix *SVD_WorkMat;
	gsl_vector *SVD_WorkVect;

	SVD_WorkMat = gsl_matrix_alloc(V->size2, V->size2);
	SVD_WorkVect = gsl_vector_alloc(V->size2);
	// printf("%s %d: %dx%d, %d\n", __func__, __LINE__, SVD_WorkMat->size1, SVD_WorkMat->size2, SVD_WorkVect->size);
	gsl_linalg_SV_decomp_mod(V, SVD_WorkMat, D, d, SVD_WorkVect);

	if (SVD_WorkMat)
	{
		gsl_matrix_free(SVD_WorkMat);
		SVD_WorkMat = NULL;
	}

	if (SVD_WorkVect)
	{
		gsl_vector_free(SVD_WorkVect);
		SVD_WorkVect = NULL;
	}
#else
	// The Jacobi method can compute singular values to higher relative accuracy than Golub-Reinsch algorithms
	gsl_linalg_SV_decomp_jacobi(V, D, d);	//?????
#endif

	// D <- diag(c(1/sqrt(d))
	for (i=0 ; i<d->size ; i++)
		gsl_vector_set(d, i, 1 / sqrt(gsl_vector_get(d, i)));

	gsl_matrix_set_zero(D);
	for (i=0 ; i<d->size ; i++)
		gsl_matrix_set(D, i, i, gsl_vector_get(d, i));

	// K <- D %*% t(U)
	gsl_matrix_transpose_memcpy(TU, V);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, D, TU, 0.0, V);

	// X1 <- K %*% X
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, V->size2);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &subMat1.matrix, XT, 0.0, X1);

	/*
	 * FAST ICA
	 */
	_A = ICA_compute(X1); //X1: compcXrows.
	/*
	 * OUTPUT
	 */
	// X <- t(x)
	gsl_matrix_transpose_memcpy(X, XT);
	MatrixDeCenter(X, mean);

	// K: colsXcompc
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, V->size2);
	gsl_matrix_transpose_memcpy(K, &subMat1.matrix);

	// w <- a %*% K; S <- w %*% X
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, V->size2);
	subMat2 = gsl_matrix_submatrix(D, 0, 0, compc, D->size2);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, _A, &subMat1.matrix, 0.0, &subMat2.matrix);

	subMat1 = gsl_matrix_submatrix(D, 0, 0, compc, D->size2);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &subMat1.matrix, XT, 0.0, X1);

	// S
	subMat1 = gsl_matrix_submatrix(X1, 0, 0, compc, X1->size2);
	subMat2 = gsl_matrix_submatrix(S, 0, 0, S->size1, compc);
	gsl_matrix_transpose_memcpy(&subMat2.matrix, &subMat1.matrix);

	// A <- t(w) %*% solve(w * t(w))
	subMat1 = gsl_matrix_submatrix(D, 0, 0, compc, compc);
	subMat2 = gsl_matrix_submatrix(TU, 0, 0, compc, compc);
	gsl_matrix_transpose_memcpy(&subMat2.matrix, &subMat1.matrix);

	subMat3 = gsl_matrix_submatrix(V, 0, 0, compc, compc);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &subMat1.matrix, &subMat2.matrix, 0.0, &subMat3.matrix);

	// Inverse matrix of V with dim of compc by compc.
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, compc);
	InvMat = MatrixInvert(&subMat1.matrix);
	if (InvMat == NULL)
	{
		ret = -1;
		goto end;
	}

	subMat1 = gsl_matrix_submatrix(TU, 0, 0, compc, compc);
	subMat2 = gsl_matrix_submatrix(V, 0, 0, compc, compc);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &subMat1.matrix, InvMat, 0.0, &subMat2.matrix);

	// A
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, compc);
	gsl_matrix_transpose_memcpy(A, &subMat1.matrix);

	// W
	gsl_matrix_transpose_memcpy(W, _A);

end:
	// cleanup
	if (XT)
		gsl_matrix_free(XT);

	if (X1)
		gsl_matrix_free(X1);

	if (V)
		gsl_matrix_free(V);

	if (D)
		gsl_matrix_free(D);

	if (TU)
		gsl_matrix_free(TU);

	if (InvMat)
		gsl_matrix_free(InvMat);

	if (scale)
		gsl_vector_free(scale);

	if (d)
		gsl_vector_free(d);

	if (mean)
		gsl_vector_free(mean);

	return ret;
}

void CLocalFeature::WriteBackMatrixToImages(gsl_matrix *x, string postfix)
{
	assert(x != NULL);
	assert(postfix.size() != 0);

	int need_sync = 0;
	int i, j, y_size, yuv_size, rows, cols;
	size_t slash, dot;
	string filename;
	FILE *out_fd = NULL;
	unsigned char pixel_data;
	gsl_vector_view x_column_view;
	gsl_matrix_uchar *xu = NULL;
	gsl_vector_uchar_view xu_column_view;

	rows = x->size1;
	cols = x->size2;

	printf("[%s] postfix %s(%d,%d)\n", __func__, postfix.c_str(), rows, cols);

	xu = gsl_matrix_uchar_alloc(rows, cols);

	// Convert double to uchar
	for (i=0 ; i<cols ; i++)
	{
		x_column_view = gsl_matrix_column(x, i);
		xu_column_view = gsl_matrix_uchar_column(xu, i);

		for (j=0 ; j<rows ; j++)
		{
			gsl_vector_uchar_set(&xu_column_view.vector, j, gsl_vector_get(&x_column_view.vector, j));
		}
	}

	for (i=0 ; i<cols && i<m_filelist[i].size() ; i++)
	{
		xu_column_view = gsl_matrix_uchar_column(xu, i);

		slash = m_filelist[i].find_last_of("/");
		dot   = m_filelist[i].find_last_of(".");

		if (slash != string::npos && dot != string::npos)
		{
			filename.clear();
			filename = m_output_folder;
			filename.append("/");
			filename.append(m_filelist[i].c_str(), slash + 1, dot - slash - 1);
			filename.append("_");
			filename.append(postfix);
			filename.append(m_filelist[i], dot, m_filelist[i].size());

			printf("fn %s\n", filename.c_str());

			if ((out_fd = fopen(filename.c_str(), "w")) != NULL)
			{
				y_size = xu_column_view.vector.size;
				yuv_size = y_size + (y_size >> 1);

				for (j=0 ; j<y_size ; j++)
				{
					pixel_data = gsl_vector_uchar_get(&xu_column_view.vector, j);
					fwrite(&pixel_data, 1, sizeof(pixel_data), out_fd);
				}

				for (j=y_size ; j<yuv_size ; j++)
				{
					pixel_data = 128;
					fwrite(&pixel_data, 1, sizeof(pixel_data), out_fd);
				}

				fclose(out_fd);
				out_fd = NULL;

				need_sync++;
			}
			else
				printf("Can not open %s\n", filename.c_str());
		}
	}

	if (need_sync)
		sync();

}
