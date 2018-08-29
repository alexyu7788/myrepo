#include "candidate.h"

void CandidateGetGeoInfo(Candidate* _candidate, uint32_t* r, uint32_t* c, uint32_t* w, uint32_t* h)
{
    *r = _candidate->m_r;
    *c = _candidate->m_c;
    *w = _candidate->m_w;
    *h = _candidate->m_h;
}

void CandidateSetGeoInfo(Candidate* _candidate, uint32_t r, uint32_t c, uint32_t w, uint32_t h)
{
    _candidate->m_r = r;
    _candidate->m_c = c;
    _candidate->m_w = w;
    _candidate->m_h = h;
}

