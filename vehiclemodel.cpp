#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utility.h"
#include "vehiclemodel.h"

CVehicleModel::CVehicleModel(FCWS__VehicleModel__Type vm_type)
{
	m_terminate = m_finish = false;

	m_vm_type = vm_type;
	m_local_feature_count = 0;

	m_filelist.clear();

	for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		m_local_feature[i] = new CLocalFeature(m_vm_type, (FCWS__Local__Type)i);
	}
}

CVehicleModel::~CVehicleModel()
{
//	printf("[%s]\n", __func__);

	m_vm_type = FCWS__VEHICLE__MODEL__TYPE__UnKonwn;
	m_local_feature_count = 0;

	for (int i=0 ; i<(int)m_filelist.size() ; i++)
	{
		m_filelist[i] = "";
	}

	m_filelist.clear();

	for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_local_feature[i])
		{
			delete m_local_feature[i];
			m_local_feature[i] = NULL;
		}
	}
}

int CVehicleModel::StartTrainingThreads()
{
	// pthread_cond_init(&m_Cond, NULL);
	// pthread_mutex_init(&m_Mutex, NULL);

	m_finish = false;

	pthread_create(&m_monitor_thread, NULL, TrainingMonitorThread, this);

	// Start Training Threads.
	for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_local_feature[i])
			pthread_create(&m_thread[i], NULL, TrainingProcess, m_local_feature[i]);
	}

	// Join Training Threads.
	for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_thread[i])
			pthread_join(m_thread[i], NULL);
	}

	m_finish = true;

	pthread_join(m_monitor_thread, NULL);

	// printf("[VehicleModel::Training Down %d] done\n", m_vm_type);

	return 0;
}

int CVehicleModel::StartDetectionThreads()
{
	// pthread_cond_init(&m_Cond, NULL);
	// pthread_mutex_init(&m_Mutex, NULL);

	m_finish = false;

	pthread_create(&m_monitor_thread, NULL, DetectionMonitorThread, this);

    memset(m_thread, 0x0, sizeof(pthread_t) * FCWS__LOCAL__TYPE__TOTAL);

	// Start Detection Threads.
	for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_local_feature[i])
        {
			pthread_create(&m_thread[i], NULL, DetectionProcess, m_local_feature[i]);
        }
	}

	// Join Training Threads.
	for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_thread[i])
			pthread_join(m_thread[i], NULL);
	}

	m_finish = true;

	pthread_join(m_monitor_thread, NULL);

	// printf("[VehicleModel] Detection  %d is done\n", m_vm_type);

	return 0;
}

int CVehicleModel::SetParam(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type, FCWS__Para__Type para_type, gsl_matrix *from)
{
	assert(from != NULL);

	m_vm_type = vm_type;

	if (m_local_feature[local_type] == NULL)
	{
		m_local_feature[local_type] = new CLocalFeature(m_vm_type, local_type);
		m_local_feature[local_type]->SetParam(local_type, para_type, from);
	}
	else
	{
//		if (m_local_feature[pos]->GetParaObj(para_type))
//			m_local_feature[pos]->DeleteParaObj(para_type);

		m_local_feature[local_type]->SetParam(para_type, from);

	}

	return 0;
}

int CVehicleModel::LoadParam(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type, FCWS__Para__Type para_type, int rows, int cols, double *from)
{
	assert(from != NULL);

//	if (from == NULL)
//	{
//		if (m_local_feature[local_type])
//		{
//			delete m_local_feature[local_type];
//			m_local_feature[local_type] = NULL;
//		}
//
//		return -1;
//	}

	m_vm_type = vm_type;

	if (m_local_feature[local_type] == NULL)
	{
		m_local_feature[local_type] = new CLocalFeature(m_vm_type, local_type);
		m_local_feature[local_type]->LoadParam(local_type, para_type, rows, cols, from);
	}
	else
	{
		m_local_feature[local_type]->LoadParam(para_type, rows, cols, from);
	}

	return 0;
}

bool CVehicleModel::LoadParam(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type, FCWS__Para2* param)
{
    if (!param)
        return false;

    m_vm_type = vm_type;

	if (m_local_feature[local_type] == NULL)
	{
		m_local_feature[local_type] = new CLocalFeature(m_vm_type, local_type);
		return m_local_feature[local_type]->LoadParam(local_type, param);
	}
	else
	{
		return m_local_feature[local_type]->LoadParam(param);
	}
}

bool CVehicleModel::HasParam()
{
    bool res = true;

    for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
    {
        if (m_local_feature[i])
            res &= m_local_feature[i]->HasParam();
        else
            res = false;
    }

    return res;
}

bool CVehicleModel::SaveParam(FCWS__Local__Type local_type, FCWS__Para__Type para_type, FCWS__Para *param)
{
	if (m_local_feature[local_type])
		return m_local_feature[local_type]->SaveParam(para_type, param);

	return false;
}


bool CVehicleModel::SaveParam(FCWS__Local__Type local_type, FCWS__Para2* para)
{
    if (m_local_feature[local_type])
        return m_local_feature[local_type]->SaveParam(para);

    return true;
}















void CVehicleModel::SetPCAAndICAComponents(int pca_first_k_components, int pca_compoments_offset, int ica_first_k_components, int ica_compoments_offset)
{
	for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_local_feature[i])
		{
			m_local_feature[i]->SetPCAAndICAComponents(pca_first_k_components, pca_compoments_offset, ica_first_k_components, ica_compoments_offset);
		}
	}
}

void CVehicleModel::SetOutputPath(string output_folder)
{
	for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_local_feature[i])
		{
			m_local_feature[i]->SetOutputPath(output_folder);
		}
	}
}

int CVehicleModel::PickUpFiles(vector<string> & feedin, int rows, int cols)
{
	char temp[MAX_FN_L];
	char *pch = NULL;
	vector<string>::iterator it;

	if (feedin.size() > 0)
	{
		m_filelist.clear();

		printf("Search Vehicle : %s\n", search_vehicle_model_pattern[(int)m_vm_type]);

	//	printf("%d %d\n", m_filelist.size(), feedin.size());

		for (it = feedin.begin(); it != feedin.end() ; it++)
		{
//			printf("%s\n", it->c_str());
			if (strlen((char*)it->c_str()))
			{
				pch = strstr((char*)it->c_str(), search_vehicle_model_pattern[(int)m_vm_type]);
				if (pch)
				{
					// printf("%s\n", it->c_str());
					m_filelist.push_back(it->c_str());
					it->clear();
				}
			}
		}

	//	printf("%d %d\n\n", m_filelist.size(), feedin.size());

		for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
		{
			if (m_local_feature[i])
			{
				m_local_feature[i]->PickUpFiles(m_filelist);
			}
		}

		m_filelist.clear();
	}

	return 0;
}

bool CVehicleModel::FinishTraining()
{
	return m_finish;
}

void CVehicleModel::Stop()
{
    for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
    {
        if (m_local_feature[i])
        {
            m_local_feature[i]->Stop();
        }
    }

    m_terminate = m_finish = true;
}

//--------------------------------------------------------------------------------------------------------
void* CVehicleModel::TrainingProcess(void *arg)
{
	int ret = 0;
	CLocalFeature* localfeature = (CLocalFeature*)arg;

	if (localfeature)
	{
		ret = localfeature->DoTraining();
	}

	return NULL;
}

void* CVehicleModel::DetectionProcess(void *arg)
{
	int ret = 0;
	CLocalFeature* localfeature = (CLocalFeature*)arg;

	if (localfeature)
	{
		ret = localfeature->DoDetection();
	}

	return NULL;
}

void* CVehicleModel::TrainingMonitorThread(void* arg)
{
	CVehicleModel* vm = (CVehicleModel*)arg;
	unsigned long long start_time, end_time;

	start_time = GetTime();

	while(!vm->m_finish)
	{
		end_time = GetTime();
		printf("[VehicleModel %d] Timelapse %llu s    \r", vm->m_vm_type, (end_time - start_time) / 1000);
		fflush(stdout);
		usleep(1000000);
	}

	return NULL;
}

void* CVehicleModel::DetectionMonitorThread(void* arg)
{
	CVehicleModel* vm = (CVehicleModel*)arg;
    bool isidle = true;

    for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
    {
        while(!vm->m_local_feature[i]->DetectionReady());
        {
            usleep(1000);
        }
    }

	while(!vm->m_finish)
	{
        // Wait all local features are finished.
        if (vm->IsIdle() == false)
        {
            usleep(10000);
            continue;
        }

        // Trigger local detection.
        for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
        {
            if (vm->m_thread[i])
                vm->m_local_feature[i]->TriggerDetection();
        }

        // Wait all local features are finished.
        while (!vm->m_finish)
        {
            //isidle = true;

            //for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
            //    isidle &= vm->m_local_feature[i]->IsIdle();

            if (vm->IsIdle())
                break;
            //else
            //    printf("vm %s is busy\n\n", search_vehicle_model_pattern[vm->m_vm_type]);

            usleep(10000);
        }

        printf("vm %s score: \n", search_vehicle_model_pattern[vm->m_vm_type]);
        for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
        {
            if (vm->m_thread[i])
                printf("score %lf\n", vm->m_local_feature[i]->GetScore());
        }
	}

	return NULL;
}

bool CVehicleModel::IsIdle()
{
    bool res = true;

    for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
    {
        if (m_local_feature[i])
            res &= m_local_feature[i]->IsIdle();
        else
            res = false;

    }

    return res;
}
