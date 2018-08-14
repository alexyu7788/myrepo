#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "vc.h"

CVehicleCandidate::CVehicleCandidate()
{
    m_r = m_c = m_w = m_h = 0;

    memset(m_score, 0x0, FCWS__VEHICLE__MODEL__TYPE__TOTAL);
    m_vm_type = FCWS__VEHICLE__MODEL__TYPE__UnKonwn;

}

CVehicleCandidate::~CVehicleCandidate()
{

}

void CVehicleCandidate::GetGeometricInfo(int &r, int &c, int &w, int &h)
{
    r = m_r;
    c = m_c;
    w = m_w;
    h = m_h;
}

void CVehicleCandidate::SetGeometricInfo(int r, int c, int w, int h)
{
    m_r = r;
    m_c = c;
    m_w = w;
    m_h = h;
}

