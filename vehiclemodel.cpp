#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utility.h"
#include "vehiclemodel.h"

CVehicleModel::CVehicleModel(FCWS__VehicleModel__Type vm_type)
{
	m_vm_type = vm_type;
	m_local_model_count = 0;
	m_finishtrain = 0;

	m_filelist.clear();

	for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		m_local_model[i] = new CLocalModel(m_vm_type, (FCWS__Local__Type)i);
	}
}

CVehicleModel::~CVehicleModel()
{
//	printf("[%s]\n", __func__);

	m_vm_type = FCWS__VEHICLE__MODEL__TYPE__UnKonwn;
	m_local_model_count = 0;

	for (int i=0 ; i<(int)m_filelist.size() ; i++)
	{
		m_filelist[i] = "";
	}

	m_filelist.clear();

	for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_local_model[i])
		{
			delete m_local_model[i];
			m_local_model[i] = NULL;
		}
	}
}

int CVehicleModel::StartTrainingThreads()
{
	// pthread_cond_init(&m_Cond, NULL);
	// pthread_mutex_init(&m_Mutex, NULL);

	m_finishtrain = 0;

	pthread_create(&m_monitor_thread, NULL, MonitorThread, this);

	// Start Training Threads.
	for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_local_model[i])
			pthread_create(&m_thread[i], NULL, TrainingProcess, m_local_model[i]);
	}

	// Join Training Threads.
	for (int i=0 ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_thread[i])
			pthread_join(m_thread[i], NULL);
	}

	m_finishtrain = 1;

	pthread_join(m_monitor_thread, NULL);

	// printf("[VehicleModel::Training Down %d] done\n", m_vm_type);

	return 0;
}

int CVehicleModel::Set(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type pos, FCWS__Para__Type para_type, gsl_matrix *from)
{
	assert(from != NULL);

	m_vm_type = vm_type;

	if (m_local_model[pos] == NULL)
	{
		m_local_model[pos] = new CLocalModel(m_vm_type, pos);
		m_local_model[pos]->Set(pos, para_type, from);
	}
	else
	{
//		if (m_local_model[pos]->GetParaObj(para_type))
//			m_local_model[pos]->DeleteParaObj(para_type);

		m_local_model[pos]->Set(para_type, from);

	}

	return 0;
}

int CVehicleModel::Set(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type pos, FCWS__Para__Type para_type, int rows, int cols, double *from)
{
	assert(from != NULL);

	m_vm_type = vm_type;

	if (m_local_model[pos] == NULL)
	{
		m_local_model[pos] = new CLocalModel(m_vm_type, pos);
		m_local_model[pos]->Set(pos, para_type, rows, cols, from);
	}
	else
	{
		m_local_model[pos]->Set(para_type, rows, cols, from);
	}

	return 0;
}

void CVehicleModel::SetPCAAndICAComponents(int pca_first_k_components, int pca_compoments_offset, int ica_first_k_components, int ica_compoments_offset)
{
	for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_local_model[i])
		{
			m_local_model[i]->SetPCAAndICAComponents(pca_first_k_components, pca_compoments_offset, ica_first_k_components, ica_compoments_offset);
		}
	}
}

void CVehicleModel::SetOutputPath(string output_folder)
{
	for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
	{
		if (m_local_model[i])
		{
			m_local_model[i]->SetOutputPath(output_folder);
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
			if (m_local_model[i])
			{
				m_local_model[i]->PickUpFiles(m_filelist);
			}
		}

		m_filelist.clear();
	}

	return 0;
}

int CVehicleModel::FinishTraining()
{
	return m_finishtrain;
}

void* CVehicleModel::TrainingProcess(void *arg)
{
	int ret = 0;
	CLocalModel* localmodel = (CLocalModel*)arg;

	if (localmodel)
	{
		ret = localmodel->DoTraining();
	}

	return NULL;
}

void* CVehicleModel::MonitorThread(void* arg)
{
	CVehicleModel* vm = (CVehicleModel*)arg;
	unsigned long long start_time, end_time;

	start_time = GetTime();

	while(!vm->m_finishtrain)
	{
		end_time = GetTime();
		printf("[VehicleModel %d] Timelapse %llu s    \r", vm->m_vm_type, (end_time - start_time) / 1000);
		fflush(stdout);
		usleep(1000000);
	}

	return NULL;
}
