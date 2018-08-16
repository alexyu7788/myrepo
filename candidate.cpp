#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "candidate.h"

// CShiftWindow
CShiftWindow::CShiftWindow()
{
    m_r = m_c = m_w = m_h = 0;
    m_score = 0.0;
}

CShiftWindow::~CShiftWindow()
{

}

void CShiftWindow::operator=(const CShiftWindow &shift_window)
{
    m_r = shift_window.m_r;
    m_c = shift_window.m_c;

    m_w = shift_window.m_w;
    m_h = shift_window.m_h;

    m_score = shift_window.m_score;
}

void CShiftWindow::GetPos(int &r, int &c)
{
    r = m_r;
    c = m_c;
}

void CShiftWindow::GetWH(int &w, int &h)
{
    w = m_w;
    h = m_h;
}

void CShiftWindow::SetPos(int r, int c)
{
    m_r = r;
    m_c = c;
}

void CShiftWindow::SetWH(int w, int h)
{
    m_w = w;
    m_h = h;
}

void CShiftWindow::GetInfo(int &r, int &c, int &w, int &h)
{
    r = m_r;
    c = m_c;
    w = m_w;
    h = m_h;
}

void CShiftWindow::SetInfo(int r, int c, int w, int h)
{
    m_r = r;
    m_c = c;
    m_w = w;
    m_h = h;
}

double CShiftWindow::GetScore()
{
    return m_score;
}

void CShiftWindow::SetScore(double score)
{
    m_score = score;
}

// CLocalInfo
CLocalInfo::CLocalInfo()
{
    m_r = m_c = m_w = m_h = 0;
}

CLocalInfo::~CLocalInfo()
{
}

void CLocalInfo::operator=(const CLocalInfo &local_info)
{
    m_r = local_info.m_r;
    m_c = local_info.m_c;

    m_w = local_info.m_w;
    m_h = local_info.m_h;

    m_shiftwindow = local_info.m_shiftwindow;
}

// local info itself
void CLocalInfo::GetPos(int &r, int &c)
{
    r = m_r;
    c = m_c;
}

void CLocalInfo::GetWH(int &w, int &h)
{
    w = m_w;
    h = m_h;
}

void CLocalInfo::SetPos(int r, int c)
{
    m_r = r;
    m_c = c;
}

void CLocalInfo::SetWH(int w, int h)
{
    m_w = w;
    m_h = h;
}

// shift window
void CLocalInfo::GetSWPos(int &r, int &c)
{
    m_shiftwindow.GetPos(r, c);

}

void CLocalInfo::GetSWWH(int &w, int &h)
{
    m_shiftwindow.GetWH(w, h);
}

void CLocalInfo::SetSWPos(int r, int c)
{
    m_shiftwindow.SetPos(r, c);
}

void CLocalInfo::SetSWWH(int r, int c)
{
    m_shiftwindow.SetWH(r, c);
}

void CLocalInfo::GetInfo(int &r, int &c, int &w, int &h)
{
    r = m_r;
    c = m_c;
    w = m_w;
    h = m_h;
}

void CLocalInfo::SetInfo(int r, int c, int w, int h)
{
    m_r = r;
    m_c = c;
    m_w = w;
    m_h = h;
}

void CLocalInfo::GetSWInfo(CShiftWindow &sw)
{
    sw = m_shiftwindow;
}

void CLocalInfo::GetSWInfo(int &r, int &c, int &w, int &h)
{
    m_shiftwindow.GetInfo(r, c, w, h);
}

void CLocalInfo::SetSWInfo(const CShiftWindow &sw)
{
    m_shiftwindow = sw;
}

void CLocalInfo::SetSWInfo(int r, int c, int w, int h)
{
    m_shiftwindow.SetInfo(r, c, w, h);
}

// CCandidate
CCandidate::CCandidate()
{
    m_r = m_c = m_w = m_h = 0;

    memset(m_score, 0x0, FCWS__VEHICLE__MODEL__TYPE__TOTAL);
    m_vm_type = FCWS__VEHICLE__MODEL__TYPE__UnKonwn;

}

CCandidate::~CCandidate()
{

}

void CCandidate::GetInfo(int &r, int &c, int &w, int &h)
{
    r = m_r;
    c = m_c;
    w = m_w;
    h = m_h;
}

void CCandidate::SetInfo(int r, int c, int w, int h)
{
    m_r = r;
    m_c = c;
    m_w = w;
    m_h = h;
}

void CCandidate::GetLocalInfo(FCWS__Local__Type local_type, int &r, int &c, int &w, int &h)
{
    if (local_type >= FCWS__LOCAL__TYPE__TOTAL)
        return;
    
    m_localinfo[local_type].GetInfo(r, c, w, h);
}

void CCandidate::SetLocalInfo(FCWS__Local__Type local_type, const CLocalInfo &local_info)
{
    m_localinfo[local_type] = local_info;
}

void CCandidate::SetLocalInfo(FCWS__Local__Type local_type, int r, int c, int w, int h)
{
    if (local_type >= FCWS__LOCAL__TYPE__TOTAL)
        return;
    
    m_localinfo[local_type].SetInfo(r, c, w, h);
}

void CCandidate::GetSWInfo(FCWS__Local__Type local_type, CShiftWindow &sw)
{
    if (local_type >= FCWS__LOCAL__TYPE__TOTAL)
        return;
    
    m_localinfo[local_type].GetSWInfo(sw);
}

void CCandidate::GetSWInfo(FCWS__Local__Type local_type, int &r, int &c, int &w, int &h)
{
    if (local_type >= FCWS__LOCAL__TYPE__TOTAL)
        return;
    
    m_localinfo[local_type].GetSWInfo(r, c, w, h);
}

void CCandidate::SetSWInfo(FCWS__Local__Type local_type, const CShiftWindow &sw)
{
    if (local_type >= FCWS__LOCAL__TYPE__TOTAL)
        return;

    m_localinfo[local_type].SetSWInfo(sw);
}

void CCandidate::SetSWInfo(FCWS__Local__Type local_type, int r, int c, int w, int h)
{
    if (local_type >= FCWS__LOCAL__TYPE__TOTAL)
        return;
    
    m_localinfo[local_type].SetSWInfo(r, c, w, h);
}
