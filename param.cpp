#include "param.h"

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

int CParam::Set(FCWS__Para__Type type, int rows, int cols, double *data)
{
	assert(rows > 0);
	assert(cols > 0);
	assert(data != NULL);

	int r, c;

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

int CParam::Set(FCWS__Para__Type type, gsl_matrix *from)
{
	assert(from != NULL);

	int r, c;

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
