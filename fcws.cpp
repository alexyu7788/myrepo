#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <error.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <SDL.h>

#include "fcws.h"
#include "utility.h"

//FCWS ------------------------------------------------------------------------------
FCWS::FCWS()
{
    m_terminate = m_one_step_mode = false;

    m_imgy = NULL;
    m_width = 0;
    m_height = 0;
    m_filelist.clear();
	m_vm_count = 0;

	for (int i=FCWS__VEHICLE__MODEL__TYPE__Compact ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		m_threads[i] = 0;
		m_vm[i] = new CVehicleModel((FCWS__VehicleModel__Type)i);
	}

	m_models	= NULL;

    //debug window
    m_debugwindow = NULL;

    // control flow
    memset(&m_cf_thread, 0, sizeof(pthread_t));
    pthread_mutex_init(&m_cf_mutex, NULL);
    pthread_cond_init(&m_cf_cond, NULL);
}

FCWS::~FCWS()
{
//	printf("[%s]\n", __func__);

	m_vm_count = 0;

	for (int i=FCWS__VEHICLE__MODEL__TYPE__Compact ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		if (m_vm)
		{
			delete m_vm[i];
			m_vm[i] = NULL;
		}
	}

	m_filelist.clear();

    if (m_debugwindow)
    {
        m_debugwindow->Terminate();

        delete m_debugwindow;
        m_debugwindow = NULL;
    }
    
    if (m_imgy)
    {
        free(m_imgy);
        m_imgy = NULL;
    }

    m_terminate = true;
}

int FCWS::LoadModel(string model_name)
{
	assert(model_name.size() != 0);

	int rtn = 0;
	int i, j, k, rows, cols;
	size_t len;
	uint8_t *buf = NULL;
	FILE *fd = NULL;
	FCWS__Models *models = NULL;
	FCWS__VehicleModel__Type vm_type;
	FCWS__Local__Type local_type;
	FCWS__Para__Type para_type;

	unsigned long long start_time, end_time;

	start_time = GetTime();

	if ((fd = fopen(model_name.c_str(), "r")) == NULL)
	{
		rtn = -1;
		goto fail;
	}

	fseek(fd, 0, SEEK_END);
	len = ftell(fd);
	rewind(fd);
	//printf("len %d\n", len);

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
		if (models->vm[i])
		{
			vm_type = models->vm[i]->type;
//			m_vm[vm_type] = new VehicleModel();

			if (m_vm[vm_type])
			{
				for (j=0 ; j<models->vm[i]->n_local ; j++)
				{
					if (models->vm[i]->local[j])
					{
						local_type = models->vm[i]->local[j]->local;

                        m_vm[vm_type]->SetLocalFeatureSWWH(local_type, models->vm[i]->local[j]->sw_w, models->vm[i]->local[j]->sw_h);
                        m_vm[vm_type]->SetLocalFeatureNeedThread(local_type, models->vm[i]->local[j]->need_thread);

						for (k=0 ; k<models->vm[i]->local[j]->n_para ; k++)
						{
							if (models->vm[i]->local[j]->para[k])
							{
								para_type = models->vm[i]->local[j]->para[k]->type;

								rows = models->vm[i]->local[j]->para[k]->rows;
								cols = models->vm[i]->local[j]->para[k]->cols;

								if (para_type != -1 && rows && cols)
								{
									printf("Loading %s of %s of %s.		\t(%d:%d)\n",
											search_param_pattern[para_type],
											search_local_model_pattern[local_type],
											search_vehicle_model_pattern[vm_type],
											rows,
											cols);

									m_vm[vm_type]->LoadParam(vm_type, local_type, para_type, rows, cols, models->vm[i]->local[j]->para[k]->data);
								}
							}
						}

                        //printf("Loading %s of %s.\n",
                        //        search_local_model_pattern[local_type],
                        //        search_vehicle_model_pattern[vm_type]);
                        m_vm[vm_type]->LoadParam(vm_type, local_type, models->vm[i]->local[j]->para2);

                        //printf("%d, %d, (%d,%d)\n",
                        //    models->vm[i]->local[j]->para2->pca->mean_size,
                        //    models->vm[i]->local[j]->para2->pca->eval_size,
                        //    models->vm[i]->local[j]->para2->pca->evec_size1,
                        //    models->vm[i]->local[j]->para2->pca->evec_size2);
                        //printf("%d, %d, (%d,%d)\n",
                        //    models->vm[i]->local[j]->para2->pca2->mean_size,
                        //    models->vm[i]->local[j]->para2->pca2->eval_size,
                        //    models->vm[i]->local[j]->para2->pca2->evec_size1,
                        //    models->vm[i]->local[j]->para2->pca2->evec_size2);
					}
				}
			}
			else
			{
				rtn = -1;
				printf("[%s] fails.\n", __func__);
			}
		}
	}

fail:

	if (fd)
		fclose(fd);

	if (buf)
		free(buf);

	if (models)
		fcws__models__free_unpacked(models, NULL);

	end_time = GetTime();
	printf("[%s] costs %llu ms.\n", __func__, end_time - start_time);

	return rtn;
}

int FCWS::SaveModel(string model_name)
{
	assert(model_name.size() != 0);

	int rtn = 0;
	int i, j, k, rows, cols;
	size_t len, wlen;
	uint8_t *buf = NULL;
	FILE *fd = NULL;
	FCWS__Models models = FCWS__MODELS__INIT;
	FCWS__VehicleModel__Type vm_type;
	FCWS__Local__Type local_type;
	FCWS__Para__Type para_type;

	models.n_vm = FCWS__VEHICLE__MODEL__TYPE__TOTAL;
	models.vm = (FCWS__VehicleModel**)malloc(sizeof(FCWS__VehicleModel*) * models.n_vm);

	for (i=FCWS__VEHICLE__MODEL__TYPE__Compact ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		models.vm[i] = (FCWS__VehicleModel*)malloc(sizeof(FCWS__VehicleModel));

		if (models.vm[i])
		{
			fcws__vehicle__model__init(models.vm[i]);

			models.vm[i]->type 		= (FCWS__VehicleModel__Type)i;
			models.vm[i]->n_local 	= FCWS__LOCAL__TYPE__TOTAL;
			models.vm[i]->local 	= (FCWS__Local**)malloc(sizeof(FCWS__Local*) * models.vm[i]->n_local);

			for (j=FCWS__LOCAL__TYPE__LEFT ; j<FCWS__LOCAL__TYPE__TOTAL ; j++)
			{
				models.vm[i]->local[j] = (FCWS__Local*)malloc(sizeof(FCWS__Local));

				if (models.vm[i]->local[j])
				{
					fcws__local__init(models.vm[i]->local[j]);

					models.vm[i]->local[j]->local 	= (FCWS__Local__Type)j;
					models.vm[i]->local[j]->n_para 	= FCWS__PARA__TYPE__TOTAL;
					models.vm[i]->local[j]->para	= (FCWS__Para**)malloc(sizeof(FCWS__Para*) * models.vm[i]->local[j]->n_para);
                    m_vm[i]->GetLocalFeatureWH((FCWS__Local__Type)j, models.vm[i]->local[j]->sw_w, models.vm[i]->local[j]->sw_h);
                    models.vm[i]->local[j]->need_thread = (j == FCWS__LOCAL__TYPE__GARBAGE ? false : true);

					for (k=FCWS__PARA__TYPE__PCA ; k<FCWS__PARA__TYPE__TOTAL ; k++)
					{
						models.vm[i]->local[j]->para[k] = (FCWS__Para*)malloc(sizeof(FCWS__Para));

						if (models.vm[i]->local[j]->para[k])
						{
							fcws__para__init(models.vm[i]->local[j]->para[k]);

							if (false == m_vm[i]->SaveParam((FCWS__Local__Type)j, (FCWS__Para__Type)k, models.vm[i]->local[j]->para[k]))
							{
								models.vm[i]->local[j]->para[k]->type = FCWS__PARA__TYPE__UnKonwn;
								models.vm[i]->local[j]->para[k]->cols = models.vm[i]->local[j]->para[k]->rows = models.vm[i]->local[j]->para[k]->n_data = 0;
								models.vm[i]->local[j]->para[k]->data = NULL;
							}
						}
					}

					models.vm[i]->local[j]->para2 = (FCWS__Para2*)malloc(sizeof(FCWS__Para2));

					if (models.vm[i]->local[j]->para2)
					{
						fcws__para2__init(models.vm[i]->local[j]->para2);

						m_vm[i]->SaveParam((FCWS__Local__Type)j, models.vm[i]->local[j]->para2);
//                        printf("Saving %s of %s.\n",
//                                search_local_model_pattern[j],
//                                search_vehicle_model_pattern[i]);
//
//                        printf("pca2=> %d,%d,(%d,%d)\n",
//                                models.vm[i]->local[j]->para2->pca2->mean_size,
//                                models.vm[i]->local[j]->para2->pca2->eval_size,
//                                models.vm[i]->local[j]->para2->pca2->evec_size1,
//                                models.vm[i]->local[j]->para2->pca2->evec_size2);
					}
				}
			}
		}
	}


	len = fcws__models__get_packed_size(&models);
	buf = (uint8_t*)malloc(sizeof(uint8_t)*len);
	fcws__models__pack(&models, buf);

	if ((fd = fopen(model_name.c_str(), "w")) == NULL)
	{
		rtn = -1;
		goto fail;
	}

	fseek(fd, 0, SEEK_SET);
	wlen = fwrite(buf, 1, len, fd);

	if (wlen == len)
		printf("[%s] Save %s successfully.\n", __func__, model_name.c_str());

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
				for (j=0 ; j<models.vm[i]->n_local ; j++)
				{
					if (models.vm[i]->local[j])
					{
						for (k=0 ; k<models.vm[i]->local[j]->n_para  ; k++ )
						{
							if (models.vm[i]->local[j]->para[k]->data)
								free(models.vm[i]->local[j]->para[k]->data);

							free(models.vm[i]->local[j]->para[k]);
						}

						free(models.vm[i]->local[j]);
					}
				}

				free(models.vm[i]);
			}
		}

		free(models.vm);
	}

	return rtn;
}

bool FCWS::LoadFiles(string infolder)
{
	DIR *dir = NULL;
	struct dirent *entry;
	string temp;

	printf("infolder %s\n", infolder.c_str());
    
	dir = opendir(infolder.c_str());

	if (dir == NULL)
	{
		printf("Can not open directory %s\n", infolder.c_str());
		return false;
	}

	m_filelist.clear();

    while ((entry = readdir(dir)) != NULL)
    {
    	if (entry->d_type & DT_REG)
    	{
    		temp.clear();
    		temp.append(infolder);
    		temp.append("/");
    		temp.append(entry->d_name);

    		m_filelist.push_back(temp);
    	}
    }

    if (m_filelist.size() == 0)
    {
    	closedir(dir);
    	dir = NULL;

		printf("There is no file in directory %s\n", infolder.c_str());
		return false;
    }

    sort(m_filelist.begin(), m_filelist.end());

	for (int i=FCWS__VEHICLE__MODEL__TYPE__Compact ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		if (m_vm[i])
			m_vm[i]->PickUpFiles(m_filelist);
	}

	closedir(dir);
	dir = NULL;

	m_filelist.clear();

	return true;
}

int FCWS::FeedFiles(vector<string> & feedin, int rows, int cols)
{
	printf("\n~~~~~~~~[FCWS::FeedFiles] Start~~~~~~~~\n");

	for (int i=FCWS__VEHICLE__MODEL__TYPE__Compact ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		if (m_vm[i])
			m_vm[i]->PickUpFiles(feedin, rows, cols);
	}

	feedin.clear();

	printf("\n~~~~~~~~[FCWS::FeedFiles] Done~~~~~~~~\n");

	return 0;
}

int FCWS::DoTraining
(	int pca_first_k_components, 
	int pca_compoments_offset, 
	int ica_first_k_components, 
	int ica_compoments_offset,
	string output_folder
)
{
	printf("\n~~~~~~~~[FCWS::DoTraining] Start~~~~~~~~\n");

	CreateOutputFolder(output_folder);

	// Start training threads.
	for (int i=0 ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		if (m_vm[i])
		{
			m_vm[i]->SetOutputPath(output_folder);
			m_vm[i]->SetPCAAndICAComponents(pca_first_k_components, 
											pca_compoments_offset, 
											ica_first_k_components, 
											ica_compoments_offset);
			pthread_create(&m_threads[i], NULL, StartTrainingThreads, m_vm[i]);
		}
	}

	// Join training threads.
	for (int i=0 ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		if (m_threads[i])
		{
			pthread_join(m_threads[i], NULL);
		}
	}

	printf("~~~~~~~~[FCWS::DoTraining] Done~~~~~~~~\n");

	return 0;
}

bool FCWS::InitDetection(string folder, bool debugwindow, bool onestep)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fpath[256];

    m_one_step_mode = onestep;

    m_filelist.clear();
    sscanf(folder.c_str(), "yuv_%d_%d", &m_width, &m_height);

    if (!m_width || !m_height)
        return false;

    m_imgy = (uint8_t*)malloc(sizeof(uint8_t) * m_width * m_height);

    if (!m_imgy)
        return false;

    if ((dir = opendir(folder.c_str())) == NULL)
        return false;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type & DT_REG)
        {
            snprintf(fpath, sizeof(fpath), "%s/%s", folder.c_str(), entry->d_name);
            m_filelist.push_back(fpath);
        }
    }

    sort(m_filelist.begin(), m_filelist.end());

    //for (std::vector<std::string>::iterator it = m_filelist.begin() ; it != m_filelist.end() ; it++)
    //{
    //    printf("%s\n", it->c_str());
    //}

    closedir(dir);
    dir = NULL;

    if (debugwindow)
        InitDebugWindow("FCWS Detection", folder, m_width, m_height);
    
    if (onestep)
    {
        string fn;
        FILE *fd = NULL;
        vector<string>::iterator it;

        if (m_filelist.size() != 0 && m_imgy)
        {
            fn = m_filelist[0];
            it = m_filelist.begin();
            fn = *it;
            m_filelist.erase(it);

            printf("Load %s\n", fn.c_str());

            if ((fd = fopen(fn.c_str(), "r")) != NULL)
            {
                memset(m_imgy , 0x0, m_width * m_height);
                fread(m_imgy, 1, m_width * m_height, fd);

                fclose(fd);
                fd = NULL;
            }
        }

        m_debugwindow->GotoNextFile();

        DoDetection(m_imgy, m_width, m_height);
    }

    // program will wait here until terminate.....
    InitDetectionThreads(debugwindow);

    return true;
}

int FCWS::InitDetectionThreads(bool debugwindow)
{
    // Start control flow
    if (debugwindow)
        pthread_create(&m_cf_thread, NULL, ControlFlow, this);

	// Start detection threads.
    memset(m_threads, 0x0, sizeof(pthread_t) * FCWS__VEHICLE__MODEL__TYPE__TOTAL);

	for (int i=0 ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		if (m_vm[i])
        {
            if (m_vm[i]->HasParam())
                pthread_create(&m_threads[i], NULL, StartDetectionThreads, m_vm[i]);
            else
            {
                delete m_vm[i];
                m_vm[i] = NULL;
            }
        }
	}


	// Join detection threads.
	for (int i=0 ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
	{
		if (m_threads[i])
			pthread_join(m_threads[i], NULL);
	}

    if (m_cf_thread)
        pthread_join(m_cf_thread, NULL);

    return 0;
}

bool FCWS::DoDetection(uint8_t *image, uint32_t width, uint32_t height)
{
	if (image == NULL || width == 0 || height == 0)
		return -1;

    // TODO
    // Hypothesis Generator is not ready.
    CCandidate *vc =  NULL;
    CandidatesIt it;

    m_vcs.clear();

    vc = new CCandidate();
    vc->SetInfo(0, 0, width, height);
    m_vcs.push_back(vc);

    // Dispatch HG with geometric infomation to each vehicle model.
	for (int i=0 ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
    {
        if (m_vm[i] && m_vm[i]->HasParam())
        {
            m_vm[i]->SetDetectionOneStep(m_one_step_mode);
            m_vm[i]->SetDetectionSource(image, width, height, m_vcs);
        }
    }

	return true;
}

bool FCWS::InitDebugWindow(string title, string folder, int w, int h)
{
    m_debugwindow = new CMainWindow(title, folder, w, h);

    if (!m_debugwindow)
        return false;

    if (m_debugwindow->Init() == false)
        return false;

    pthread_create(&m_event_thread, NULL, DWProcessEvent, this);

    return true;
}

void* FCWS::StartTrainingThreads(void *arg)
{
	CVehicleModel* vm = (CVehicleModel*)arg;

	vm->StartTrainingThreads();

	return NULL;
}

void* FCWS::StartDetectionThreads(void *arg)
{
	CVehicleModel* vm = (CVehicleModel*)arg;

	vm->StartDetectionThreads();

	return NULL;
}

void FCWS::CreateOutputFolder(string out)
{
	struct stat st;
	char cmd[256];

	if (stat(out.c_str(), &st) == 0)
	{
		snprintf(cmd, sizeof(cmd), "rm -f %s/*", out.c_str());
		system(cmd);
	}
	else
		mkdir(out.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
}

void* FCWS::DWProcessEvent(void* arg)
{
    FCWS* pThis = (FCWS*)arg;

    pthread_detach(pthread_self());

    while (!pThis->m_terminate)
    {
        if (pThis->m_debugwindow)
        {
            pThis->m_debugwindow->ProcessEvent();

            if (pThis->m_debugwindow->IsQuit())
            {
                pThis->m_terminate = true;
                pThis->m_debugwindow->Terminate();

                for (int i=0 ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
                {
                    if (pThis->m_vm[i])
                        pThis->m_vm[i]->Stop();
                }

                // wakeup control flow
                pthread_mutex_lock(&pThis->m_cf_mutex);
                pthread_cond_signal(&pThis->m_cf_cond);
                pthread_mutex_unlock(&pThis->m_cf_mutex);
            }
            else if (pThis->m_debugwindow->BtnPressed_n())
            {
                pThis->m_debugwindow->GotoNextFile();

                // wakeup control flow
                pthread_mutex_lock(&pThis->m_cf_mutex);
                pthread_cond_signal(&pThis->m_cf_cond);

                pThis->m_cf_next_file = true;

                pthread_mutex_unlock(&pThis->m_cf_mutex);
            }
            else if (pThis->m_debugwindow->BtnPressed_s())
            {
                // wakeup control flow
                pthread_mutex_lock(&pThis->m_cf_mutex);
                pthread_cond_signal(&pThis->m_cf_cond);

                pThis->m_cf_next_step = true;

                pthread_mutex_unlock(&pThis->m_cf_mutex);
            }
        }

        usleep(100000);
    }

    return NULL;
}

void* FCWS::ControlFlow(void* arg)
{
    FCWS* pThis = (FCWS*)arg;
    int w,h;
    string fn;
    FILE *fd = NULL;
    vector<string>::iterator it;

    w = pThis->m_width;
    h = pThis->m_height;

    while (!pThis->m_terminate)
    {
        pthread_mutex_lock(&pThis->m_cf_mutex);
        pthread_cond_wait(&pThis->m_cf_cond, &pThis->m_cf_mutex);

        if (pThis->m_terminate)
        {
            pthread_mutex_unlock(&pThis->m_cf_mutex);
            break;
        }

        if (pThis->m_cf_next_file)
        {
            pThis->m_cf_next_file = false;

            if (pThis->m_filelist.size() != 0 && pThis->m_imgy)
            {
                fn = pThis->m_filelist[0];
                it = pThis->m_filelist.begin();
                fn = *it;
                pThis->m_filelist.erase(it);

                printf("Load %s\n", fn.c_str());

                if ((fd = fopen(fn.c_str(), "r")) != NULL)
                {
                    memset(pThis->m_imgy , 0x0, w * h);
                    fread(pThis->m_imgy, 1, w * h, fd);

                    pThis->DoDetection(pThis->m_imgy, w, h);

                    fclose(fd);
                    fd = NULL;
                }
            }
        }
        else if (pThis->m_cf_next_step)
        {
            pThis->m_cf_next_step = false;

            for (int i=0 ; i<FCWS__VEHICLE__MODEL__TYPE__TOTAL ; i++)
            {
                if (pThis->m_vm[i] && pThis->m_vm[i]->HasParam())
                {
                    pThis->m_vm[i]->TriggerDetectionOneStep(pThis->m_vcs);
                    pThis->m_debugwindow->UpdateDrawInfo(pThis->m_vcs);
                }
            }
        }

        pthread_mutex_unlock(&pThis->m_cf_mutex);
    }

    return NULL;
}


































