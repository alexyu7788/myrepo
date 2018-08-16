#include <string.h>
#include <unistd.h>
#include "utility.h"
#include "param.h"

CParam2::CParam2(FCWS__Local__Type local_type)
{
	m_local_type = local_type;
    m_pca = m_pca2 = NULL;
}

CParam2::~CParam2()
{
    if (m_pca)
    {
        if (m_pca->mean_data)
        {
            gsl_vector_free(m_pca->mean_data);
            m_pca->mean_data = NULL;
        }

        if (m_pca->eval_data)
        {
            gsl_vector_free(m_pca->eval_data);
            m_pca->eval_data = NULL;
        }

        if (m_pca->evec_data)
        {
            gsl_matrix_free(m_pca->evec_data);
            m_pca->evec_data = NULL;
        }

        free(m_pca);
        m_pca = NULL;
    }

    if (m_pca2)
    {
        if (m_pca2->mean_data)
        {
            gsl_vector_free(m_pca2->mean_data);
            m_pca2->mean_data = NULL;
        }

        if (m_pca2->eval_data)
        {
            gsl_vector_free(m_pca2->eval_data);
            m_pca2->eval_data = NULL;
        }

        if (m_pca2->evec_data)
        {
            gsl_matrix_free(m_pca2->evec_data);
            m_pca2->evec_data = NULL;
        }

        free(m_pca2);
        m_pca2 = NULL;
    }
}

bool CParam2::SetPCAParam(gsl_vector* mean, gsl_vector* eval, gsl_matrix* evec)
{
    if (m_pca)
    {
        if (m_pca->mean_data)
        {
            gsl_vector_free(m_pca->mean_data);
            m_pca->mean_data = NULL;
        }

        if (m_pca->eval_data)
        {
            gsl_vector_free(m_pca->eval_data);
            m_pca->eval_data = NULL;
        }

        if (m_pca->evec_data)
        {
            gsl_matrix_free(m_pca->evec_data);
            m_pca->evec_data = NULL;
        }

        free(m_pca);
        m_pca = NULL;
    }

    m_pca = (PCA*)malloc(sizeof(PCA));
    memset(m_pca, 0x0, sizeof(PCA));

    return SetPCAParam(m_pca, mean, eval, evec);
}

bool CParam2::SetPCA2Param(gsl_vector* mean, gsl_vector* eval, gsl_matrix* evec)
{
    if (m_pca2)
    {
        if (m_pca2->mean_data)
        {
            gsl_vector_free(m_pca2->mean_data);
            m_pca2->mean_data = NULL;
        }

        if (m_pca2->eval_data)
        {
            gsl_vector_free(m_pca2->eval_data);
            m_pca2->eval_data = NULL;
        }

        if (m_pca2->evec_data)
        {
            gsl_matrix_free(m_pca2->evec_data);
            m_pca2->evec_data = NULL;
        }

        free(m_pca2);
        m_pca2 = NULL;
    }

    m_pca2 = (PCA*)malloc(sizeof(PCA));
    memset(m_pca2, 0x0, sizeof(PCA));

    return SetPCAParam(m_pca2, mean, eval, evec);
}


bool CParam2::SaveParam(FCWS__Para2* param)
{
    if (!param)
        return false;

    if (param->pca)
    {
        free(param->pca);
        param->pca = NULL;
    }

    if (param->pca2)
    {
        free(param->pca2);
        param->pca2 = NULL;
    }

    if (param->ica)
    {
        free(param->ica);
        param->ica = NULL;
    }

    // Even if there is no any data for this param, it MUST allocate memory & initialize
    // for this param & all members.
    // It can not free memory on this param & all members.
    if ((param->pca = (FCWS__Para2__Pca*)malloc(sizeof(FCWS__Para2__Pca))) == NULL)
        return false;

    fcws__para2__pca__init(param->pca);

    if ((param->pca2 = (FCWS__Para2__Pca*)malloc(sizeof(FCWS__Para2__Pca))) == NULL)
       return false;

    fcws__para2__pca__init(param->pca2);

    if ((param->ica = (FCWS__Para2__Ica*)malloc(sizeof(FCWS__Para2__Ica))) == NULL)
        return false;

    fcws__para2__ica__init(param->ica);

    InitAndSetPCAMember(param->pca, m_pca);
    
    InitAndSetPCAMember(param->pca2, m_pca2);

    InitAndSetICAMember(param->ica, m_pca);

    return true;
}


bool CParam2::LoadParam(FCWS__Para2* param)
{
    if (!param || !param->pca || !param->pca2)
        return false;

    if (!param->pca->mean_size || !param->pca->eval_size ||
        !param->pca->evec_size1 || !param->pca->evec_size2)
        return false;

    if (!param->pca2->mean_size || !param->pca2->eval_size ||
        !param->pca2->evec_size1 || !param->pca2->evec_size2)
        return false;

    int i, j;
    int size1, size2;
    
    // PCA
    if ((m_pca = (PCA*)malloc(sizeof(PCA))) == NULL)
        return false;

    // Mean
    size1 = param->pca->mean_size;

    if ((m_pca->mean_data = gsl_vector_alloc(size1)) == NULL)
        return false;
    
    m_pca->mean_size = size1;
    for (i=0 ; i<size1 ; i++)
        gsl_vector_set(m_pca->mean_data, i, param->pca->mean_data[i]);
    
    //Eval 
    size1 = param->pca->eval_size;

    if ((m_pca->eval_data = gsl_vector_alloc(size1)) == NULL)
        return false;
    
    m_pca->eval_size = size1;
    for (i=0 ; i<size1 ; i++)
        gsl_vector_set(m_pca->eval_data, i, param->pca->eval_data[i]);

    //Evec
    size1 = param->pca->evec_size1;
    size2 = param->pca->evec_size2;

    if ((m_pca->evec_data = gsl_matrix_alloc(size1, size2)) == NULL)
        return false;

    m_pca->evec_size1 = size1;
    m_pca->evec_size2 = size2;
    for (i=0 ; i<size1 ; i++)
        for (j=0 ; j<size2 ; j++)
            gsl_matrix_set(m_pca->evec_data, i, j, param->pca->evec_data[i * size2 + j]);

    // PCA2
    if ((m_pca2 = (PCA*)malloc(sizeof(PCA))) == NULL)
        return false;

    // Mean
    size1 = param->pca2->mean_size;

    if ((m_pca2->mean_data = gsl_vector_alloc(size1)) == NULL)
        return false;
    
    m_pca2->mean_size = size1;
    for (i=0 ; i<size1 ; i++)
        gsl_vector_set(m_pca2->mean_data, i, param->pca2->mean_data[i]);
    
    //Eval 
    size1 = param->pca2->eval_size;

    if ((m_pca2->eval_data = gsl_vector_alloc(size1)) == NULL)
        return false;
    
    m_pca2->eval_size = size1;
    for (i=0 ; i<size1 ; i++)
        gsl_vector_set(m_pca2->eval_data, i, param->pca2->eval_data[i]);

    //Evec
    size1 = param->pca2->evec_size1;
    size2 = param->pca2->evec_size2;

    if ((m_pca2->evec_data = gsl_matrix_alloc(size1, size2)) == NULL)
        return false;

    m_pca2->evec_size1 = size1;
    m_pca2->evec_size2 = size2;
    for (i=0 ; i<size1 ; i++)
        for (j=0 ; j<size2 ; j++)
            gsl_matrix_set(m_pca2->evec_data, i, j, param->pca2->evec_data[i * size2 + j]);

    return true;
}

#define PI 3.14159

double CParam2::CalProbability(const gsl_vector *img)
{
    double res = 0;

    if (!img || !m_pca || !m_pca2)
    {
        printf("[%s][%d] Error.\n", __func__, __LINE__);
        return 0;
    }

    if (!m_pca->mean_size || !m_pca->eval_size || !m_pca->evec_size1 || !m_pca->evec_size2)
    {
        printf("[%s][%d] Error.\n", __func__, __LINE__);
        return 0;
    }

    if (!m_pca2->mean_size || !m_pca2->eval_size || !m_pca2->evec_size1 || !m_pca2->evec_size2)
    {
        printf("[%s][%d] Error.\n", __func__, __LINE__);
        return 0;
    }

    if (img->size != m_pca->mean_size || img->size != m_pca->evec_size1)
    {
        printf("[%s][%d] Error. (%d, %d, %d)\n", __func__, __LINE__, img->size, m_pca->mean_size, m_pca->evec_size1);
        return 0;
    }

    if (img->size != m_pca2->mean_size || img->size != m_pca2->evec_size1)
    {
        printf("[%s][%d] Error. (%d, %d, %d)\n", __func__, __LINE__, img->size, m_pca2->mean_size, m_pca2->evec_size1);
        return 0;
    }

    res = CalProbabilityOfPCA(img, m_pca);
    //CalProbabilityOfPCA(img, m_pca2);

    return res;
}

//-----------------------------------------------------------------------------------
bool CParam2::SetPCAParam(PCA *pca, gsl_vector* mean, gsl_vector* eval, gsl_matrix* evec)
{
    if (!pca || !mean || !eval || !evec)
        return false;

    //Mean
    if (pca->mean_data)
    {
        gsl_vector_free(pca->mean_data);
        pca->mean_data = NULL;
    }

    if ((pca->mean_data = gsl_vector_alloc(mean->size)) == NULL)
    {
        goto fail;
    }

    pca->mean_size = mean->size;
    gsl_vector_memcpy(pca->mean_data, mean);

    //Eval
    if (pca->eval_data)
    {
        gsl_vector_free(pca->eval_data);
        pca->eval_data = NULL;
    }

    if ((pca->eval_data = gsl_vector_alloc(eval->size)) == NULL)
    {
        goto fail;
    }

    pca->eval_size = eval->size;
    gsl_vector_memcpy(pca->eval_data, eval);

    //Evec
    if (pca->evec_data)
    {
        gsl_matrix_free(pca->evec_data);
        pca->evec_data = NULL;
    }

    if ((pca->evec_data = gsl_matrix_alloc(evec->size1, evec->size2)) == NULL)
    {
        goto fail;
    }

    pca->evec_size1 = evec->size1;
    pca->evec_size2 = evec->size2;
    gsl_matrix_memcpy(pca->evec_data, evec);

    return true;

fail:

    if (pca->mean_data)
    {
        gsl_vector_free(pca->mean_data);
        pca->mean_data = NULL;
    }

    if (pca->eval_data)
    {
        gsl_vector_free(pca->eval_data);
        pca->eval_data = NULL;
    }

    if (pca->evec_data)
    {
        gsl_matrix_free(pca->evec_data);
        pca->evec_data = NULL;
    }

    return false;
}

bool CParam2::InitAndSetPCAMember(FCWS__Para2__Pca *pca, PCA *mpca)
{
    if(!pca || !mpca || !mpca->mean_data || !mpca->eval_data || !mpca->evec_data)
        return false;

    int i, j;
    int size1, size2;

    //Mean
    size1 = pca->mean_size = pca->n_mean_data = mpca->mean_size;

    if ((pca->mean_data = (double*)malloc(sizeof(double) * size1)) == NULL)
        goto fail;

    for (i=0 ; i<size1 ; i++)
        pca->mean_data[i] = gsl_vector_get(mpca->mean_data, i);

    //Eval
    size1 = pca->eval_size = pca->n_eval_data = mpca->eval_size;

    if ((pca->eval_data = (double*)malloc(sizeof(double) * size1)) == NULL)
        goto fail;

    for (i=0 ; i<size1 ; i++)
        pca->eval_data[i] = gsl_vector_get(mpca->eval_data, i);

    //Evec
    size1 = pca->evec_size1 = mpca->evec_size1;
    size2 = pca->evec_size2 = mpca->evec_size2;
    pca->n_evec_data = size1 * size2;

    if ((pca->evec_data = (double*)malloc(sizeof(double) * size1 * size2)) == NULL)
        goto fail;

    for (i=0 ; i<size1 ; i++)
        for (j=0 ; j<size2 ; j++)
            pca->evec_data[i * size2 + j] = gsl_matrix_get(mpca->evec_data, i, j);

    return true;

fail:
    
    if (pca->mean_data)
    {
        free(pca->mean_data);
        pca->mean_data = NULL;
    }

    if (pca->eval_data)
    {
        free(pca->eval_data);
        pca->eval_data = NULL;
    }

    if (pca->evec_data)
    {
        free(pca->evec_data);
        pca->evec_data = NULL;
    }

    return false;
}

bool CParam2::InitAndSetICAMember(FCWS__Para2__Ica *ica, PCA *mpca)
{
    if (!ica || !mpca)
        return false;


    return true;

fail:
    

    return false;
}

double CParam2::CalProbabilityOfPCA(const gsl_vector *img, PCA *pca)
{
    int i;
    double numerator = 1, denominator = 1;
    double res = 0, temp = 1;
    double *y = NULL, y_sum = 0;
    double eval;
    gsl_vector *I;
    gsl_vector_view evec_col_view;

    //printf("\n\n[%s] %s\n", __func__, 
    //            search_local_model_pattern[m_local_type]);

    // ---------------------PCA--------------------------------------------
    // Denominator
    for (i=0 ; i<pca->eval_size ; i++)
        temp *= pow(gsl_vector_get(pca->eval_data, i), 0.5);

    printf("temp %lf, %lf\n", temp, pow( 2*PI , pca->eval_size * 0.5 ));
    denominator = temp * pow( 2*PI , pca->eval_size * 0.5);
    //printf("denominator %lf\n", denominator); 

    // Numerator

    // I^ = I - mean
    if ((I = gsl_vector_alloc(img->size)) == NULL)
    {
        printf("[%s][%d] Error.");
        return 0;
    }

    gsl_vector_memcpy(I, img);
    gsl_vector_sub(I, pca->mean_data);

    if ((y = (double*)malloc(sizeof(double) * pca->evec_size2)) == NULL)
    {
        printf("[%s][%d] Error.");
        return 0;
    }

    for (i=0 ; i<pca->evec_size2 ; i++)
    {
        // Uk-T * I^
        evec_col_view = gsl_matrix_column(pca->evec_data, i);
        gsl_blas_ddot(&evec_col_view.vector, I, &y[i]);

        eval = gsl_vector_get(pca->eval_data, i);

        if (eval != 0)
        {
            printf("y[%d]=%lf, eval data[%d]=%lf\n", i, y[i], i, gsl_vector_get(pca->eval_data, i));
            y_sum += (y[i] / eval);
            printf("y sum = %lf\n", y_sum);
        }
    }

    // exp[-1/2*y_sum]
    numerator = exp(0 - (y_sum * 0.5));

    res = numerator / denominator;
    printf("res %lf, %lf/%lf\n", res, numerator , denominator);

    if (I)
    {
        gsl_vector_free(I);
        I = NULL;
    }

    if (y)
    {
        free(y);
        y = NULL;
    }

    return res;
}

// Para ------------------------------------------------------------------------------
CParam::CParam(FCWS__Para__Type para_type)
{
	m_para_type = para_type;
	m_data_count = 0;
	m_data = NULL;
}

CParam::~CParam()
{
//	printf("[%s]\n", __func__);

	m_para_type = FCWS__PARA__TYPE__UnKonwn;
	m_data_count = 0;

	if (m_data)
	{
		gsl_matrix_free(m_data);
		m_data = NULL;
	}
}

int CParam::LoadParam(FCWS__Para__Type type, int rows, int cols, double *data)
{
	assert(rows > 0);
	assert(cols > 0);
	assert(data != NULL);

	unsigned int r, c;

	if (m_data)
		gsl_matrix_free(m_data);

	if ((m_data = gsl_matrix_alloc(rows, cols)) != NULL)
	{
		for (r=0 ; r<rows ; r++)
			for (c=0 ; c<cols ; c++)
				gsl_matrix_set(m_data, r, c, data[r*cols+c]);
	}

	m_data_count = rows * cols;
	m_para_type = type;

	return 0;
}

int CParam::SetParam(FCWS__Para__Type type, gsl_matrix *from)
{
	assert(from != NULL);

	unsigned int r, c;

	r = from->size1;
	c = from->size2;

	if (m_data)
		gsl_matrix_free(m_data);

	if ((m_data = gsl_matrix_alloc(from->size1, from->size2)) != NULL)
	{
		gsl_matrix_memcpy(from, m_data);
	}

	m_data_count = r * c;
	m_para_type = type;

	return 0;
}


bool CParam::SaveParam(FCWS__Para *param)
{
	assert(param != NULL);

	unsigned int r, c;

	if (m_data == NULL)
	{
//		printf("[%s][%s] There is no any data yet.\n", __FILE__, __func__);
		return false;
	}

	param->type = m_para_type;
	param->rows = m_data->size1;
	param->cols = m_data->size2;
	param->n_data = (m_data->size1 * m_data->size2);
	param->data = (double*)malloc(sizeof(double) * param->n_data);

	if (param->data == NULL)
	{
		printf("[%s][%s] Can not allocate memory.\n", __FILE__, __func__);
		return false;
	}

	for (r=0 ; r<m_data->size1 ; r++)
	{
		for (c=0 ; c<m_data->size2 ; c++)
		{
			param->data[r*m_data->size2 + c] = gsl_matrix_get(m_data, r, c);
		}
	}

	return true;
}

FCWS__Para__Type CParam::GetType()
{
	return m_para_type;
}

void CParam::SetType(FCWS__Para__Type type)
{
	m_para_type = type;
}

void CParam::SetData(int row, int col, double d)
{
	if (m_data_count == 0 || m_data == NULL)
		return;

	if (row >= m_data->size1 || col >= m_data->size2)
		return;

	gsl_matrix_set(m_data, row, col, d);
}

double CParam::GetData(int row, int col)
{
	if (m_data_count == 0 || m_data == NULL)
		return 0xFFFFFFFF;

	if (row >= m_data->size1 || col >= m_data->size2)
		return 0xFFFFFFFF;

	return gsl_matrix_get(m_data, row, col);
}


//Para* Para::GetObj()
//{
//	return this;
//}







//---------------------------------------------------------------------
