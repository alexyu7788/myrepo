#include "utility.h"

//#include <iostream>
//#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <google/protobuf/io/zero_copy_stream_impl.h>
//#include <google/protobuf/io/zero_copy_stream.h>
//#include "models2.pb.h"
#include "models2.pb-c.h"

#ifdef PROTOBUF_proto_2fmodels2_2eproto_INCLUDED
using namespace std;
using namespace FCWS;
using namespace google::protobuf::io;
#endif

unsigned long long get_time()
{
	struct timeval now_time;

	gettimeofday(&now_time, NULL);

	return ((unsigned long long)now_time.tv_sec * 1000) + (now_time.tv_usec / 1000);;
}

void print_matrix(char *name, gsl_matrix *m)
{
	int i, j;

	assert(name != NULL);
	assert(m != NULL);

	printf("%s\n", name);

	for (i=0 ; i<m->size1 ; i++)
	{
		for (j=0 ; j<m->size2 ; j++)
			printf("%.6lf\t", gsl_matrix_get(m, i, j));
		printf("\n");
	}

	printf("\n");
}

void print_vector(char *name, gsl_vector *v)
{
	int i;

	assert(name != NULL);
	assert(v != NULL);

	printf("%s\n", name);

	for (i=0 ; i<v->size ; i++)
		printf("%.2lf\t", gsl_vector_get(v, i));

	printf("\n\n");
}

void print_matrix_float(char *name, gsl_matrix_float *m)
{
	int i, j;

	assert(name != NULL);
	assert(m != NULL);

	printf("%s\n", name);

	for (i=0 ; i<m->size1 ; i++)
	{
		for (j=0 ; j<m->size2 ; j++)
			printf("%.6lf\t", gsl_matrix_float_get(m, i, j));
		printf("\n");
	}

	printf("\n");
}

void print_vector_float(char *name, gsl_vector_float *v)
{
	int i;

	assert(name != NULL);
	assert(v != NULL);

	printf("%s\n", name);

	for (i=0 ; i<v->size ; i++)
		printf("%.6lf\t", gsl_vector_float_get(v, i));

	printf("\n\n");
}

void write_matrix_to_images(gsl_matrix *x, char *dst_folder, char **file_list, char *postfix)
{
	assert(x != NULL);
	assert(dst_folder != NULL);
	assert(file_list != NULL);
	assert(postfix != NULL);

	int i, j, y_size, yuv_size, rows, cols;
	char fn[MAX_FN_L] = {0}, ext[MAX_FN_L] = {0};
	char filename[MAX_FN_L] = {0}, *pch;
	FILE *out_fd = NULL;
	unsigned char pixel_data;
	gsl_vector_view x_column_view;
	gsl_matrix_uchar *xu = NULL;
	gsl_vector_uchar_view xu_column_view;

	rows = x->size1;
	cols = x->size2;

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

	for (i=0 ; i<cols ; i++)
	{
		xu_column_view = gsl_matrix_uchar_column(xu, i);

		pch = strrchr(file_list[i], '.');
		strncpy(fn, file_list[i], pch - file_list[i]);
		strcpy(ext, pch);
		memset(filename, 0, sizeof(char) * MAX_FN_L);
		snprintf(filename, MAX_FN_L, "%s/%s_%s%s", dst_folder, fn, postfix, ext);

		if ((out_fd = fopen(filename, "w")) != NULL)
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
		}
	}
}

gsl_matrix *load_image_to_matrix(char *src_folder, char **file_list, int file_count, int w, int h)
{
	assert(file_count > 0);
	assert(file_list != NULL);

	int i, j, y_size, uv_size, yuv_size;
	char file_name[MAX_FN_L] = {0};
	unsigned char *temp = NULL, pixel_data;
	FILE *fd = NULL;
	gsl_vector_uchar_view image_y;
	gsl_matrix *image_matrix = NULL;
	gsl_vector_view image_matrix_column_view;

	y_size = w * h;
	uv_size = y_size >> 1;
	yuv_size = y_size + uv_size;
	temp = (unsigned char*)malloc(sizeof(unsigned char) * y_size);
	image_matrix = gsl_matrix_alloc(y_size, file_count);

	for (i=0 ; i<file_count ; i++)
	{
		memset(file_name, 0, sizeof(*file_name));
		snprintf(file_name, MAX_FN_L, "%s_%d_%d/%s", src_folder, w, h, file_list[i]);

		if ((fd = fopen(file_name, "r")) != NULL)
		{
			fread(temp, 1, y_size, fd);
			image_y = gsl_vector_uchar_view_array(temp, y_size);
			image_matrix_column_view = gsl_matrix_column(image_matrix, i);

			for (j=0 ; j<image_matrix_column_view.vector.size ; j++)
			{
    			pixel_data = gsl_vector_uchar_get(&image_y.vector, j);
    			gsl_vector_set(&image_matrix_column_view.vector, j, pixel_data);
			}

			fclose(fd);
			fd = NULL;

// debug...
//			for (j = 0 ; j < img_y_view.vector.size ; j++)
//			{
//				pixel_data = (uint8_t)gsl_vector_uchar_get(&img_y_view.vector, j);
//				fwrite((uint8_t*)&pixel_data, 1, sizeof(pixel_data), out_fd);
//			}
//
//			for (j = y_size ; j < yuv_size ; j++)
//			{
//				pixel_data = 128;
//				fwrite((uint8_t*)&pixel_data, 1, sizeof(pixel_data), out_fd);
//			}
		}
		else
		{
			if (image_matrix)
				gsl_matrix_free(image_matrix);

			break;
		}
	}

	if (temp)
		free(temp);

	return image_matrix;
}

//int save_to_json(char *json_filename, gsl_matrix *m)
//{
//	assert(json_filename != NULL);
//	assert(m != NULL);
//
//	int i, j;
//	int rtn = 0, rows, cols;
//	char *strings = NULL;
//	FILE *fd = NULL;
//	cJSON *parameters = NULL;
//	cJSON *model1 = NULL, *model2 = NULL;
//	cJSON *ary = NULL;
//
//	rows = m->size1;
//	cols = m->size2;
//
//	if ((fd = fopen(json_filename, "w")) != NULL)
//	{
//		if ((parameters = cJSON_CreateArray()) == NULL)
//		{
//			fprintf(stderr, "Can not open parameters object.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		fprintf(stdout, "Create model object successfully.\n");
//
//		//---------------------------------------------------------------------
//		if ((model1 = cJSON_AddObjectToObject(parameters, "model1")) == NULL)
//		{
//			fprintf(stderr, "Can not create model1.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_AddStringToObject(model1, "position", "All cars") == NULL)
//		{
//			fprintf(stderr, "Can not add string to object.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_AddNumberToObject(model1, "rows", rows) == NULL)
//		{
//			fprintf(stderr, "Can not add rows to object.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_AddNumberToObject(model1, "cols", cols) == NULL)
//		{
//			fprintf(stderr, "Can not add cols to object.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if ((ary = cJSON_CreateDoubleArray(m->data, rows*cols)) == NULL)
//		{
//			fprintf(stderr, "Can not create double array.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		cJSON_AddItemToObject(model1, "pca", ary);
//
//		//---------------------------------------------------------------------
//		if ((model2 = cJSON_AddObjectToObject(parameters, "model2")) == NULL)
//		{
//			fprintf(stderr, "Can not create model1.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_AddStringToObject(model2, "position", "Left of cars") == NULL)
//		{
//			fprintf(stderr, "Can not add string to object.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_AddNumberToObject(model2, "rows", rows) == NULL)
//		{
//			fprintf(stderr, "Can not add rows to object.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_AddNumberToObject(model2, "cols", cols) == NULL)
//		{
//			fprintf(stderr, "Can not add cols to object.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//
//
//
//		//---------------------------------------------------------------------
//		strings = cJSON_Print(parameters);
//		if (strings)
//		{
//			fwrite(strings, sizeof(char), strlen(strings), fd);
//		}
//		else
//		{
//			fprintf(stderr, "Can not get string from object.\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		fclose(fd);
//		fd = NULL;
//	}
//	else
//	{
//		fprintf(stderr, "Can not open %s.\n", json_filename);
//		rtn = -1;
//		goto fail;
//	}
//
//fail:
//
//	if (parameters)
//		cJSON_Delete(parameters);
//
//	if (fd)
//		fclose(fd);
//
//	return rtn;
//}
//
//int load_model_from_json_description(char *json_filename, gsl_matrix **m)
//{
//	assert(json_filename != NULL);
//
//	int i, j, k, rows, cols, rtn;
//	int parameters_size;
//	FILE *fd = NULL;
//	gsl_matrix *pm = NULL;
//	size_t read_count;
//	char *json_strings = NULL;
//	long size;
//	cJSON *parameters = NULL;
//	cJSON *models;
//	unsigned long long start_time, end_time;
//
//	if ((fd = fopen(json_filename, "r")) == NULL)
//	{
//		rtn = -1;
//		goto fail;
//	}
//
//	fseek(fd, 0, SEEK_END);
//	size = ftell(fd);
//	rewind(fd);
//	fprintf(stdout, "size %d\n", size);
//
//	if ((json_strings = (char*)malloc(sizeof(char)*size)) == NULL)
//	{
//		rtn = -1;
//		goto fail;
//	}
//
//	read_count = fread(json_strings, 1, size, fd);
//	fprintf(stdout, "read_count %d\n", read_count);
//
//	if (read_count != size)
//	{
//		rtn = -1;
//		goto fail;
//	}
//
//	json_strings[read_count] = '\0';
////	fprintf(stdout, "%s\n", json_strings);
//	start_time = get_time();
//
//	if ((parameters = cJSON_Parse(json_strings)) == NULL)
//	{
//		rtn = -1;
//		goto fail;
//	}
//	end_time = get_time();
//	printf("Time costs to parser %llu ms\n", (end_time - start_time));
//
//	if (cJSON_IsArray(parameters) == 0)
//	{
//		fprintf(stderr, "Is is not a array.\n");
//		rtn = -1;
//		goto fail;
//	}
//
//	parameters_size = cJSON_GetArraySize(parameters);
//	printf("Size of parameter size : %d\n", parameters_size);
//
//	for (i=0 ; i<parameters_size ; i++)
//	{
//		int ary_size;
//		cJSON *obj, *obj2;
//		models = cJSON_GetArrayItem(parameters, i);
//
//		if ((obj = cJSON_GetObjectItemCaseSensitive(models, "position")) == NULL)
//		{
//			printf("Can not get object[%d].", i);
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_IsString(obj))
//			printf("position: %s\n", obj->valuestring);
//
//		if ((obj = cJSON_GetObjectItemCaseSensitive(models, "rows")) == NULL)
//		{
//			printf("Can not get rows object\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_IsNumber(obj))
//		{
//			rows = (int)obj->valuedouble;
//			printf("rows %d\n", rows);
//		}
//
//		if ((obj = cJSON_GetObjectItemCaseSensitive(models, "cols")) == NULL)
//		{
//			printf("Can not get cols object\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_IsNumber(obj))
//		{
//			cols = (int)obj->valuedouble;
//			printf("cols %d\n", cols);
//		}
//
//		if ((obj = cJSON_GetObjectItemCaseSensitive(models, "pca")) == NULL)
//		{
//			printf("Can not get pca object\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		if (cJSON_IsArray(obj) == 0)
//		{
//			printf("Not an array\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		ary_size = cJSON_GetArraySize(obj);
//		printf("ary_size: %d\n", ary_size);
//
//		if (*m)
//		{
//			gsl_matrix_free(*m);
//			*m = NULL;
//		}
//
//		if ((pm = gsl_matrix_alloc(rows, cols)) == NULL)
//		{
//			printf("Can not alloc pm\n");
//			rtn = -1;
//			goto fail;
//		}
//
//		start_time = get_time();
//		for (j=0 ; j<rows ; j++)
//		{
//			for (k=0 ; k<cols ; k++)
//			{
//				obj2 = cJSON_GetArrayItem(obj, j*cols + k);
//				gsl_matrix_set(pm, j, k, obj2->valuedouble);
//			}
//		}
//		end_time = get_time();
//		printf("Time costs to copy %llu ms\n", (end_time - start_time));
//
//		*m = pm;
//	}
//
//fail:
//
//	if (fd)
//		fclose(fd);
//
//	if (json_strings)
//		free(json_strings);
//
//	if (parameters)
//		cJSON_Delete(parameters);
//
//	return rtn;
//
//}

int model_save(char *model_name, gsl_matrix *m)
{
#ifdef PROTOBUF_C_proto_2fmodels2_2eproto__INCLUDED
	assert(model_name != NULL);
	assert(m != NULL);

	int rtn = 0;
	int i, j, k, rows, cols;
	size_t len;
	uint8_t *buf = NULL;
	FILE *fd = NULL;
	unsigned long long start_time, end_time;

	FCWS__Models models = FCWS__MODELS__INIT;

	models.n_vm = FCWS__VEHICLE__MODEL__TYPE__TOTAL;
	models.vm = (FCWS__VehicleModel**)malloc(sizeof(FCWS__VehicleModel*) * models.n_vm);

	for (i=FCWS__VEHICLE__MODEL__TYPE__Compact ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		models.vm[i] = (FCWS__VehicleModel*)malloc(sizeof(FCWS__VehicleModel));
		fcws__vehicle__model__init(models.vm[i]);

		if (models.vm[i])
		{
//			printf("models.vm[%d]\n", i);
			models.vm[i]->type = (_FCWS__VehicleModel__Type)i;
			models.vm[i]->n_position = FCWS__POSITION__TYPE__TOTAL;
			models.vm[i]->position = (FCWS__Position**)malloc(sizeof(FCWS__Position*) * models.vm[i]->n_position);

			for (j=FCWS__POSITION__TYPE__LEFT ; j<FCWS__POSITION__TYPE__TOTAL ; j++)
			{
				models.vm[i]->position[j] = (FCWS__Position*)malloc(sizeof(FCWS__Position));
				fcws__position__init(models.vm[i]->position[j]);

				if (models.vm[i]->position[j])
				{
//					printf("models.vm[%d]->position[%d]\n", i, j);
					models.vm[i]->position[j]->pos = (_FCWS__Position__Type)j;
					models.vm[i]->position[j]->n_para = FCWS__PARA__TYPE__TOTAL;
					models.vm[i]->position[j]->para = (FCWS__Para**)malloc(sizeof(FCWS__Para*) * models.vm[i]->position[j]->n_para);

					for (k=FCWS__PARA__TYPE__PCA ; k<FCWS__PARA__TYPE__TOTAL ; k++)
					{
						models.vm[i]->position[j]->para[k] = (FCWS__Para*)malloc(sizeof(FCWS__Para));
						fcws__para__init(models.vm[i]->position[j]->para[k]);

						if (models.vm[i]->position[j]->para[k])
						{
//							printf("models.vm[%d]->position[%d]->para[%d]\n", i, j, k);
							models.vm[i]->position[j]->para[k]->para_type = (_FCWS__Para__Type)k;
							models.vm[i]->position[j]->para[k]->rows = m->size1;
							models.vm[i]->position[j]->para[k]->cols = m->size2;
							models.vm[i]->position[j]->para[k]->n_data = (m->size1 * m->size2);
							models.vm[i]->position[j]->para[k]->data = (double*)malloc(sizeof(double)*models.vm[i]->position[j]->para[k]->n_data);

							if (models.vm[i]->position[j]->para[k]->data)
							{
//								printf("models.vm[%d]->position[%d]->para[%d]->data\n", i, j, k);
								for (int r=0 ; r<m->size1 ; r++)
								{
									for (int c=0 ; c<m->size2 ; c++)
									{
										models.vm[i]->position[j]->para[k]->data[r*m->size2+c] = gsl_matrix_get(m, r, c);
									}
								}
							}
						}
					}
				}
			}
		}

	}

	start_time = get_time();

	len = fcws__models__get_packed_size(&models);
	buf = (uint8_t*)malloc(sizeof(uint8_t)*len);
	fcws__models__pack(&models, buf);

	if ((fd = fopen(model_name, "w")) == NULL)
	{
		rtn = -1;
		goto fail;
	}

	fseek(fd, 0, SEEK_SET);
	fwrite(buf, 1, len, fd);

fail:

	if (fd)
		fclose(fd);

	if (buf)
		free(buf);

	if (models.n_vm > 0 && models.vm)
	{
//		printf("models.n_vm %d\n", models.n_vm);

		for (i=0 ; i<models.n_vm ; i++)
		{
//			printf("models.vm[%d]\n", i);
			if (models.vm[i])
			{
				for (j=0 ; j<models.vm[i]->n_position ; j++)
				{
					if (models.vm[i]->position[j])
					{
						for (k=0 ; k<models.vm[i]->position[j]->n_para  ; k++ )
						{
							if (models.vm[i]->position[j]->para[k]->data)
								free(models.vm[i]->position[j]->para[k]->data);

							free(models.vm[i]->position[j]->para[k]);
						}

						free(models.vm[i]->position[j]);
					}
				}
				free(models.vm[i]);
			}
		}
		free(models.vm);
	}

	end_time = get_time();

	printf("%s costs %llu ms.\n", __func__, end_time - start_time);

	return rtn;


#elif (defined PROTOBUF_proto_2fmodels2_2eproto_INCLUDED)

	int rtn = 0;
	int i, j, k;
	FCWS::Models *m_models = new FCWS::Models;
	FCWS::Vehicle_Model *m_vm;
	FCWS::Position *m_pos;
	FCWS::Para *m_para;

	m_models->Clear();

//	m_vm = m_models->add_vm();
//	m_pos = m_vm->add_position();
//	m_para = m_pos->add_para();
//	m_para->set_rows(m->size1);
//	m_para->set_cols(m->size2);
//
//	m_para->add_var(1);
//	m_para->add_var(2);
//	m_para->add_var(3);
//	printf("%d:%d %d\n", m_para->rows(), m_para->cols(), m_para->var_size());
//
//	for (i=0 ; i<m_para->var_size() ; i++)
//		printf("[%d]=%lf\n", i, m_para->var(i));
	for (i=0 ; i<Vehicle_Model_Type_Type_ARRAYSIZE ; i++)
	{
		m_vm = m_models->add_vm();
		if (m_vm)
		{
			m_vm->Clear();
			m_vm->set_type((::FCWS::Vehicle_Model_Type)i);

			for (j=0 ; j<Position_Type_Type_ARRAYSIZE ; j++)
			{
				m_pos = m_vm->add_position();
				if (m_pos)
				{
					m_pos->Clear();
					m_pos->set_pos((::FCWS::Position_Type)j);

					for (k=0 ; k<Para_Type_Type_ARRAYSIZE ; k++)
					{
						m_para = m_pos->add_para();
						if (m_para)
						{
							m_para->Clear();
							m_para->set_para_type((::FCWS::Para_Type)k);
							m_para->set_rows(m->size1);
							m_para->set_cols(m->size2);
							for (int r=0 ; r<m->size1 ; r++)
							{
								for (int c=0 ; c<m->size2 ; c++)
								{
									m_para->add_data(gsl_matrix_get(m, r, c));
								}
							}

							printf("[%d] size %d\n", k, m_para->data_size());
						}
					}
				}
			}

			printf("=========[%d]=======\n", i);
		}
	}


	int fd = open(model_name, O_CREAT | O_WRONLY);
	ZeroCopyOutputStream* raw_output = new FileOutputStream(fd);
	CodedOutputStream *output = new CodedOutputStream(raw_output);

	m_models->ByteSizeLong();
	m_models->SerializeWithCachedSizes(output);

	if (m_models)
	{
		printf("Delete m_models: %d,%d\n", m_models->ByteSizeLong() >> 20, m_models->vm_size());
		delete m_models;
	}

	google::protobuf::ShutdownProtobufLibrary();

	return rtn;


#endif
}

int model_load(char *model_name, gsl_matrix **m1, gsl_matrix **m2)
{
#ifdef PROTOBUF_C_proto_2fmodels2_2eproto__INCLUDED

	assert(model_name != NULL);

	int rtn = 0;
	int i, j, k, rows, cols;
	size_t len;
	uint8_t *buf = NULL;
	FILE *fd = NULL;
	gsl_matrix *pm1 = NULL, *pm2 = NULL;
	FCWS__Models *models = NULL;

	unsigned long long start_time, end_time;

	start_time = get_time();

	if (*m1)
		gsl_matrix_free(*m1);

	if (*m2)
		gsl_matrix_free(*m2);

	if ((fd = fopen(model_name, "r")) == NULL)
	{
		rtn = -1;
		goto fail;
	}

	fseek(fd, 0, SEEK_END);
	len = ftell(fd);
	rewind(fd);
//	printf("len %d\n", len);

	if ((buf = (uint8_t*)malloc(sizeof(uint8_t)*len)) == NULL)
	{
		rtn = -1;
		goto fail;
	}

	fread(buf, 1, len, fd);
	if ((models = fcws__models__unpack(NULL, len, buf)) == NULL)
	{
		rtn = -1;
		goto fail;
	}

	for (i=0 ; i<models->n_vm ; i++)
	{
		printf("models->vm[%d]->type=%d\n", i, models->vm[i]->type);

		for (j=0 ; j<models->vm[i]->n_position ; j++)
		{
			if (models->vm[i]->position[j])
			{
				printf("models->vm[%d]->position[%d]->pos=%d\n", i, j, models->vm[i]->position[j]->pos);

				for (k=0 ; k<models->vm[i]->position[j]->n_para ; k++)
				{
					if (models->vm[i]->position[j]->para[k])
					{
						printf("models->vm[%d]->position[%d]->para[%d]->para_type=%d\n", i, j, k, models->vm[i]->position[j]->para[k]->para_type);
						printf("models->vm[%d]->position[%d]->para[%d]->rows=%d\n", i, j, k, models->vm[i]->position[j]->para[k]->rows);
						printf("models->vm[%d]->position[%d]->para[%d]->cols=%d\n", i, j, k, models->vm[i]->position[j]->para[k]->cols);
						printf("models->vm[%d]->position[%d]->para[%d]->n_data=%d\n", i, j, k, models->vm[i]->position[j]->para[k]->n_data);

						rows = models->vm[i]->position[j]->para[k]->rows;
						cols = models->vm[i]->position[j]->para[k]->cols;

						if (!pm1 && ((pm1 = gsl_matrix_alloc(rows, cols)) != NULL))
						{
							for (int r=0 ; r<rows ; r++)
							{
								for (int c=0 ; c<cols ; c++)
								{
									gsl_matrix_set(pm1, r, c, models->vm[i]->position[j]->para[k]->data[r*cols+c]);
								}

							}

							printf("pm1:%dX%d\n", pm1->size1, pm1->size2);
						}

						if (!pm2 && ((pm2 = gsl_matrix_alloc(rows, cols)) != NULL))
						{
							for (int r=0 ; r<rows ; r++)
							{
								for (int c=0 ; c<cols ; c++)
								{
									gsl_matrix_set(pm2, r, c, models->vm[i]->position[j]->para[k]->data[r*cols+c]);
								}

							}

							printf("pm2:%dX%d\n", pm2->size1, pm2->size2);
						}
					}
				}
			}
		}

	}



//
//	for (i=0 ; i<models->n_model ; i++)
//	{
//		printf("[%d]: %s %d %d %d %d %08X %08X\n", i,
//				models->model[i]->position,
//				models->model[i]->rows,
//				models->model[i]->cols,
//				models->model[i]->n_pca,
//				models->model[i]->n_ica,
//				models->model[i]->pca,
//				models->model[i]->ica
//		);
//	}
//
//	rows = models->model[0]->rows;
//	cols = models->model[0]->cols;
//
//	if ((pm1 = gsl_matrix_alloc(rows, cols)) == NULL)
//	{
//		rtn = -1;
//		goto fail;
//	}
//
//	for (i=0 ; i<rows ; i++)
//	{
//		for (j=0 ; j<cols ; j++)
//		{
////			printf("%s: [%d][%d] = %lf\n", __func__, i, j, models->model[0]->pca[i*cols+j]);
//			gsl_matrix_set(pm1, i, j, models->model[0]->pca[i*cols+j]);
//		}
//	}
//
//	rows = models->model[1]->rows;
//	cols = models->model[1]->cols;
//
//	if ((pm2 = gsl_matrix_alloc(rows, cols)) == NULL)
//	{
//		rtn = -1;
//		goto fail;
//	}
//
//	for (i=0 ; i<rows ; i++)
//	{
//		for (j=0 ; j<cols ; j++)
//			gsl_matrix_set(pm2, i, j, models->model[1]->ica[i*cols+j]);
//	}
//
	*m1 = pm1;
	*m2 = pm2;
//
fail:

	if (fd)
		fclose(fd);

	if (buf)
		free(buf);

	if (models)
		fcws__models__free_unpacked(models, NULL);

	end_time = get_time();
	printf("%s costs %llu ms.\n", __func__, end_time - start_time);

	return rtn;

#elif (defined PROTOBUF_proto_2fmodels2_2eproto_INCLUDED)

	int i,j,k,l;
	FCWS::Models *m_models = new FCWS::Models;
	FCWS::Vehicle_Model *m_vm;
	FCWS::Position *m_pos;
	FCWS::Para *m_para;
//	::google::protobuf::RepeatedField< double >* data;

	m_models->Clear();

	int fd = open(model_name, O_RDONLY);
	ZeroCopyInputStream* raw_input = new FileInputStream(fd);
	CodedInputStream *input = new CodedInputStream(raw_input);

	m_models->MergePartialFromCodedStream(input);

	printf("%d %d\n", m_models->ByteSizeLong(), m_models->vm_size());

	for (i=0 ; i<m_models->vm_size() ; i++)
	{
//		const ::FCWS::Vehicle_Model& m_vm = m_models->vm(i);;
//
//		if (m_vm.has_type())
//		{
//			printf("vm[%d]: type %d\n", i, m_vm.type());
//		}
		m_vm = m_models->mutable_vm(i);

		if (m_vm && m_vm->has_type())
		{
			printf("vm[%d]: type %d\n", i, m_vm->type());

			for (j=0 ; j<m_vm->position_size() ; j++)
			{
				m_pos = m_vm->mutable_position(j);

				if (m_pos && m_pos->has_pos())
				{
					printf("pos[%d]: pos %d\n", j, m_pos->pos());

					for (k=0 ; k<m_pos->para_size() ; k++)
					{
						m_para = m_pos->mutable_para(k);

						if (m_para && m_para->has_para_type())
						{
							printf("para[%d]: type %d(%d,%d,%d)\n", k, m_para->para_type(), m_para->rows(), m_para->cols(), m_para->data_size());

							for (l=0 ; l<m_para->data_size() ; l++)
							{
//								data = m_para->mutable_data();
								printf("[%d]=%lf\n", l, m_para->data(l));
							}
						}
					}
				}
			}

			printf("====================\n");
		}
	}

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
#endif
}
