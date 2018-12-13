#include "fcws.hpp"
 
#define BLOB_MARGIN 3

// ------------------protected function---------
BOOL CFCWS::PixelInROI(uint32_t r, uint32_t c, const roi_t* roi)
{
    float slopl, slopr;

    if (!roi) {
        fcwsdbg();
        return FALSE;
    }

    if (r < roi->point[ROI_LEFTTOP].r || r > roi->point[ROI_LEFTBOTTOM].r)
        return FALSE;

    if (c < roi->point[ROI_LEFTBOTTOM].c || c > roi->point[ROI_RIGHTBOTTOM].c)
        return FALSE;
    
    slopl = (roi->point[ROI_LEFTTOP].r - roi->point[ROI_LEFTBOTTOM].r) / (float)(roi->point[ROI_LEFTTOP].c - roi->point[ROI_LEFTBOTTOM].c);
    slopr = (roi->point[ROI_RIGHTTOP].r - roi->point[ROI_RIGHTBOTTOM].r) / (float)(roi->point[ROI_RIGHTTOP].c - roi->point[ROI_RIGHTBOTTOM].c);

    if (c >= roi->point[ROI_LEFTBOTTOM].c && c <= roi->point[ROI_LEFTTOP].c && 
        (((int)c - roi->point[ROI_LEFTBOTTOM].c == 0) || (((int)r - roi->point[ROI_LEFTBOTTOM].r) / (float)((int)c - roi->point[ROI_LEFTBOTTOM].c)) < slopl))
        return FALSE;

    if (c >= roi->point[ROI_RIGHTTOP].c && c <= roi->point[ROI_RIGHTBOTTOM].c && 
        (((int)c - roi->point[ROI_RIGHTBOTTOM].c == 0) || (((int)r - roi->point[ROI_RIGHTBOTTOM].r) / (float)((int)c - roi->point[ROI_RIGHTBOTTOM].c)) > slopr))
        return FALSE;

    return TRUE;
    
}

BOOL CFCWS::ApplyStaticROI(gsl_matrix* src, const roi_t* roi)
{
    uint32_t r, c;

    if (!src || !roi) {
        fcwsdbg();
        return FALSE;
    }

    for (r = 0 ; r < src->size1 ; ++r) {
        for (c=0 ; c< src->size2 ; ++c) {
           if (PixelInROI(r, c, roi) == FALSE)
               gsl_matrix_set(src, r, c, 255);
        }
    }

    return TRUE;
}

void CFCWS::BlobDump(string description, list<blob_t>& blobs)
{
    fcwsdbg("%s", description.c_str());

    for (auto& blob:blobs) {
        fcwsdbg("blob at (%d, %d) with (%d, %d)",
                blob.r,
                blob.c,
                blob.w,
                blob.h);
    }
}

BOOL CFCWS::BlobConvertToVC(uint32_t cols, list<blob_t>& blobs, list<candidate_t>& cands)
{
    candidate_t cand;
    int left_idx, right_idx, bottom_idx;
    int vehicle_startr, vehicle_startc;
    int vehicle_width, vehicle_height;
    int id = 0;

    if (!blobs.size()) {
        fcwsdbg();
        return FALSE;
    }

    for (auto& blob:blobs) {
        if (blob.valid == TRUE) {
            bottom_idx      = blob.r;
            
            //scale up searching region of blob with 1/4 time of width on the left & right respectively.
            left_idx        = (blob.c - (blob.w >> 2));
            right_idx       = (blob.c + blob.w + (blob.w >> 2));

            left_idx        = (left_idx < 0 ? 0 : left_idx);
            right_idx       = (right_idx >= cols ? cols - 1 : right_idx);

            vehicle_width   = (right_idx - left_idx + 1);

            if ((bottom_idx - (blob.w * 0.5)) < 0) {
                vehicle_startr = 0;
                vehicle_height = bottom_idx;
            } else {
                vehicle_height = blob.w * 0.5;
                vehicle_startr = bottom_idx - vehicle_height;
            }

            vehicle_startc  = left_idx;

            if (left_idx + vehicle_width >= cols) {
                right_idx       = cols - 1;
                vehicle_width   = right_idx - left_idx + 1;
            } 

            memset(&cand, 0x0, sizeof(candidate_t));

            cand.m_valid    = TRUE;
            cand.m_id       = id++;
            cand.m_r        = vehicle_startr;
            cand.m_c        = vehicle_startc;
            cand.m_w        = vehicle_width;
            cand.m_h        = vehicle_height;

            cands.push_back(cand);
        }
    }

    return TRUE;
}

BOOL CFCWS::BlobRearrange(list<blob_t>& blobs)
{
    int32_t diff, number = 0;
    list<blob_t>::iterator cur, next, pre, temp;

    if (!blobs.size())
        return FALSE;

redo:

    for (cur = blobs.begin() ; cur != blobs.end() ; ) {
        for (next = std::next(cur, 1) ; next != blobs.end() ; ) {

            // next blob is above cur blob.
            if (next->r < cur->r) {
                /*
                 *  a:   next     ------
                 *       cur   ------------- (delete next)
                 *
                 *  b:   next  --------
                 *       cur      ---------- (delete next)
                 *
                 *  c:   next  -----------
                 *       cur     ------      (copy next to cur and delete next)
                 *
                 */
                if ((/*(a)*/next->c >= cur->c && next->c <= cur->c + cur->w) ||
                    (/*(b)*/next->c < cur->c && next->c + next->w > cur->c && next->c + next->w <= cur->c + cur->w)) {
                    next =  blobs.erase(next);
                } else if (/*(c)*/cur->c > next->c && cur->c + cur->w <= next->c + next->w) {
                    if ((cur->r - next->r) < (cur->w * 0.5)) {

                        cur->valid  = next->valid;
                        cur->number = next->number;
                        cur->r      = next->r;
                        cur->c      = next->c;
                        cur->w      = next->w;
                        cur->h      = next->h;
                        cur->density= next->density;

                        next = blobs.erase(next);
                        //fcwsdbg("redo");
                        goto redo;
                    }
                }
            }
            // next blob is below cur blob.
            else if (next->r > cur->r) {
                /*
                 *  d:   cur     ------             -------
                 *       next   ------------- or  -------        (copy next to cur and delete next)
                 *
                 *  e:   cur     --------
                 *       next      ----------                    (copy next to cur and delete next)
                 */
                if (/*(d)*/(cur->c >= next->c && cur->c <= next->c + next->w) ||
                    /*(e)*/(cur->c < next->c && cur->c + cur->w > next->c && cur->c + cur->w <= next->c + next->w)) {
                        cur->valid  = next->valid;
                        cur->number = next->number;
                        cur->r      = next->r;
                        cur->c      = next->c;
                        cur->w      = next->w;
                        cur->h      = next->h;
                        cur->density= next->density;

                        next = blobs.erase(next);
                        //fcwsdbg("redo");
                        goto redo;
                }
            }
            // next overlaps cur.
            else if (next->r == cur->r) {
                /*
                 *  f:        next       cur 
                 *       -----------==|-------        (merge cur & next)
                 *
                 *  g:        next    3 pixels   cur 
                 *       -----------|<------->|-------      (merge cur & next)
                 *
                 *  h:        cur       next 
                 *       -----------==|-------        (merge cur & next)
                 *
                 *  i:        cur    3 pixels   next 
                 *       -----------|<------->|-------      (merge cur & next)
                 */
                if (/*(f)*/(cur->c > next->c && cur->c <= next->c + next->w) || 
                    /*(g)*/(cur->c > next->c && cur->c > next->c + next->w && abs(cur->c - next->c - next->w) < 3) || 
                    /*(h)*/(cur->c < next->c && cur->c + cur->w >= next->c)  || 
                    /*(i)*/(cur->c < next->c && cur->c + cur->w < next->c && abs(cur->c + cur->w - next->c) < 3)) {
                    // Merge cur & next
                    if (cur->c > next->c) {
                        diff    = next->c + next->w - cur->c;
                        cur->c  = next->c;
                        cur->w  = cur->w + next->w - diff;
                    } else {
                        diff    = cur->c + cur->w - next->c;
                        cur->w  = cur->w + next->w - diff;
                    }

                    next = blobs.erase(next);

                    goto redo;
                }
            }

            ++next;
        }

        ++cur;
    }

    for (auto& it:blobs) {
        it.number = number++;
    }

    return TRUE;
}

BOOL CFCWS::BlobFindIdentical(list<blob_t>& blobs, 
                               blob_t& temp)
{
    list<blob_t>::iterator it;

    for (it = blobs.begin() ; it != blobs.end() ; ++it) {
        if (it->r == temp.r && it->c == temp.c &&
            it->w == temp.w && it->h == temp.h)
            return TRUE;
    }

    return FALSE;
}

BOOL CFCWS::BlobGenerate(const gsl_matrix* src,
                  uint32_t peak_idx,
                  list<blob_t>& blobs)
{
    uint32_t sm_r, sm_c, sm_w, sm_h;
    uint32_t blob_pixel_cnt, max_blob_pixel_cnt;

    int32_t r, c;
    int32_t blob_r_top, blob_r_bottom, blob_c, blob_w, blob_h;
    int32_t max_blob_r, max_blob_c, max_blob_w, max_blob_h;
    int32_t stop_wall_cnt;
    float blob_pixel_density;
    BOOL has_neighborhood = FALSE;
    blob_t tblob;
    gsl_matrix_view sm;

    if (!src) {
        fcwsdbg();
        return FALSE;
    }

    if (peak_idx > BLOB_MARGIN + 1 && peak_idx < (src->size1 - BLOB_MARGIN - 1)) {

        sm_r = peak_idx - BLOB_MARGIN;
        sm_c = 0;
        sm_w = src->size2;
        sm_h = BLOB_MARGIN * 2;
        sm   = gsl_matrix_submatrix((gsl_matrix*)src, sm_r, sm_c, sm_h, sm_w);

        blob_pixel_cnt = max_blob_pixel_cnt = 0;

        max_blob_r = max_blob_c = max_blob_w = max_blob_h = 
        blob_c = blob_w = blob_h = -1;
        blob_r_top = INT_MAX; blob_r_bottom = INT_MIN;
        blob_c = INT_MIN;

        for (c = 1 ; c < sm.matrix.size2 - 1 ; ++c) {
            for (r = 1 ; r < sm.matrix.size1 - 1 ; ++r) {
                if (r == 1) {
                    stop_wall_cnt = sm_h - 2;
                    has_neighborhood = FALSE;
                }

                if (gsl_matrix_get(&sm.matrix, r, c) != 255) {
                    
                    if (gsl_matrix_get(&sm.matrix, r-1, c+1) != 255 ||
                        gsl_matrix_get(&sm.matrix,   r, c+1) != 255 ||
                        gsl_matrix_get(&sm.matrix, r+1, c+1) != 255)
                        has_neighborhood = TRUE;

                    blob_pixel_cnt++;

                    if (r <= blob_r_top)
                        blob_r_top = r;

                    if (r >= blob_r_bottom)
                        blob_r_bottom = r;

                    if (blob_c == INT_MIN)
                        blob_c = c;

                    if (gsl_matrix_get(&sm.matrix, r-1, c+1) == 255 && 
                        gsl_matrix_get(&sm.matrix,   r, c+1) == 255 && 
                        gsl_matrix_get(&sm.matrix, r+1, c+1) == 255 &&
                        has_neighborhood == FALSE) {

examine:
                        if (blob_pixel_cnt > 1) {
                            blob_w = c - blob_c + 1;
                            blob_h = blob_r_bottom - blob_r_top + 1;
                            blob_pixel_density = blob_pixel_cnt / (float)(blob_w * blob_h);

                            if (blob_h > 1 &&
                                ((peak_idx <= src->size1 / 3.0 && blob_w >= 10 && blob_pixel_density >= 0.2) ||
                                (peak_idx > src->size1 / 3.0 && blob_w >= 20 && blob_pixel_density >= 0.55))) {
                                tblob.valid = TRUE;
                                tblob.r     = peak_idx;
                                tblob.c     = sm_c + blob_c;
                                tblob.w     = blob_w;
                                tblob.h     = blob_h;

                                if (BlobFindIdentical(blobs, tblob) == FALSE)
                                    blobs.push_back(tblob);
                                else
                                    memset(&tblob, 0x0, sizeof(blob_t));
                            }
                        }

                        blob_pixel_cnt = 0;
                        blob_c = blob_w = blob_h = -1;
                        blob_r_top = INT_MAX; blob_r_bottom = INT_MIN;
                        blob_c = INT_MIN;
                        blob_pixel_density = 0;
                    }
                } else {
                    if (has_neighborhood == FALSE && --stop_wall_cnt <= 1) {
                        goto examine;
                    }
                }
            }
        }
    }

    return TRUE;
}

void CFCWS::VCDump(string description, list<candidate_t>& cands)
{
    if (cands.size()) {
        fcwsdbg("%s", description.c_str());
        for (auto& cand:cands) {
            if (cand.m_valid == TRUE) {
                fcwsdbg("vc[%d] at (%d, %d) with (%d, %d)",
                        cand.m_id,
                        cand.m_r,
                        cand.m_c,
                        cand.m_w,
                        cand.m_h);
            }
        }
    }
}

double CFCWS::VCGetDist(double pixel)
{
    // d = W * f / w 
    return (VehicleWidth * (EFL / 2.0)) / (pixel * PixelSize);
}

BOOL CFCWS::VCCheckByAR(list<candidate_t>& cands)
{
    double ar;

    for (auto& cand:cands) {
        if (cand.m_valid == TRUE) {
            ar = cand.m_w / (double)cand.m_h;

            if (AR_LB < ar && ar < AR_HB)
                cand.m_valid = TRUE;
            else
                cand.m_valid = FALSE;
        }
    }

    return TRUE;
}

BOOL CFCWS::VCCheckByVerticalEdge(const gsl_matrix* vedgeimg,
                                       list<candidate_t>& cands)
{
    uint32_t r, c;
    uint32_t pixel_cnt = 0, area; 
    float percentage;
    gsl_matrix_view submatrix;

    if (!vedgeimg) {
        fcwsdbg();
        return FALSE;
    }

    for (auto& cand:cands) {
        if (cand.m_valid == TRUE) {
            area = cand.m_w * cand.m_h;
            submatrix = gsl_matrix_submatrix((gsl_matrix*)vedgeimg, cand.m_r, cand.m_c, cand.m_h, cand.m_w);

            for (r=0 ; r<submatrix.matrix.size1 ; ++r) {
                for (c=0 ; c<submatrix.matrix.size2 ; ++c) {
                    if (gsl_matrix_get(&submatrix.matrix, r, c)) {
                        pixel_cnt++;
                    }
                }
            }

            percentage = pixel_cnt / (float)area;

            //fcwsdbg("VerticalEdge: %d / %d = %.04f", pixel_cnt, area, percentage);

            if (percentage < 0.2)
                cand.m_valid = FALSE;
        }
    }

    return TRUE;
}

BOOL CFCWS::VCCheck(const gsl_matrix* imgy,
                         const gsl_matrix* vedgeimg,
                         list<candidate_t>& cands)
{
    if (!imgy || !vedgeimg) {
        fcwsdbg();
        return FALSE;
    }

    VCCheckByVerticalEdge(vedgeimg, cands);

    VCCheckByAR(cands);

    return TRUE;
}

BOOL CFCWS::VCUpdateShapeByStrongVerticalEdge(const gsl_matrix* vedgeimg, list<candidate_t>& cands)
{
    uint32_t r, c;
    uint32_t rr, cc;
    uint32_t max_vedge_c, max_vedge_c_left;
    uint32_t pix_cnt, column_cnt;
    uint32_t max_edge_idx;
    int32_t  left_idx, right_idx, bottom_idx;
    double mean, sd, delta;
    double magnitude;
    double max_edge_value;
    double vedge_strength, max_vedge_strength;
    gsl_matrix_view sm, block;
    gsl_vector* strong_edge_value = NULL;

    if (!vedgeimg) {
        fcwsdbg();
        return FALSE;
    }

    for (auto& cand:cands) {
        if (cand.m_valid == TRUE) {

            sm = gsl_matrix_submatrix((gsl_matrix*)vedgeimg,
                                      cand.m_r,
                                      cand.m_c,
                                      cand.m_h,
                                      cand.m_w);

            CheckOrReallocVector(&strong_edge_value, sm.matrix.size2, TRUE);

            bottom_idx = cand.m_r + cand.m_h;

            column_cnt  =
            pix_cnt     =
            magnitude   =
            sd          = 
            mean        = 0;

            for (c = 0 ; c < sm.matrix.size2 - 2 ; ++c) {
                block = gsl_matrix_submatrix(&sm.matrix,    
                                             0,
                                             c,
                                             sm.matrix.size1,
                                             2);

                pix_cnt = magnitude = 0;

                for (rr = (block.matrix.size1 / 2) ; rr < block.matrix.size1 ; ++rr) {
                    for (cc = 0 ; cc < block.matrix.size2 ; ++cc) {
                        ++pix_cnt;
                        magnitude += gsl_matrix_get(&block.matrix, rr, cc);
                    }
                }

                vedge_strength = (magnitude / (double)(pix_cnt));
                gsl_vector_set(strong_edge_value, c, vedge_strength);
                //fcwsdbg("strong edge[%d] = %lf", c, vedge_strength);

                column_cnt++;
                mean += vedge_strength;
            }

            // mean & standart deviation
            mean /= column_cnt;

            for (c = 0 ; c < sm.matrix.size2 - 2 ; ++c) {
                delta = (mean - gsl_vector_get(strong_edge_value, c));
                sd   += (delta * delta);
            }

            sd = sqrt(sd / (double)(column_cnt -1));

            //fcwsdbg("%f, %f, %f", mean, sd, delta);
            // --------------Find left max-----------------------------------------
            max_edge_idx    =
            max_edge_value  = 0;

            for ( c = 0 ; c < strong_edge_value->size / 2 ; ++c) {
                if ((gsl_vector_get(strong_edge_value, c) > (mean + 2 * sd)) &&
                    (max_edge_value < gsl_vector_get(strong_edge_value, c))) {
                    max_edge_value = gsl_vector_get(strong_edge_value, c);
                    max_edge_idx   = c;
                }
            }

            if (max_edge_idx == 0) {
                cand.m_valid = FALSE;
                continue;
            }

            left_idx    = max_edge_idx;
            cand.m_c   += max_edge_idx;

            // --------------Find right max-----------------------------------------
            max_edge_idx    = 
            max_edge_value  = 0;

            for (c = strong_edge_value->size / 2 ; c < (strong_edge_value->size - 2); ++c) {
                if ((gsl_vector_get(strong_edge_value, c) > (mean + 2 * sd)) && 
                    (max_edge_value < gsl_vector_get(strong_edge_value, c))) {
                    max_edge_value = gsl_vector_get(strong_edge_value, c);
                    max_edge_idx   = c;
                }
            }

            if (max_edge_idx == 0) {
                cand.m_valid = FALSE;
                continue;
            }

            cand.m_w = max_edge_idx - left_idx + 1;
            cand.m_h = (cand.m_w * VHW_RATIO > bottom_idx) ? bottom_idx : cand.m_w * VHW_RATIO;
            cand.m_r = bottom_idx - cand.m_h;
            cand.m_valid = TRUE;
        }
    }

    FreeVector(&strong_edge_value);

    return TRUE;
}

BOOL CFCWS::VCUpdateHeatMap(gsl_matrix* map,
                            gsl_matrix_char* id,
                            list<candidate_t>& cands)
{
    BOOL pixel_hit = FALSE;
    uint32_t i, r, c;
    double val;

    if (!map) {
        fcwsdbg();
        return FALSE;
    }

    CheckOrReallocMatrixChar(&id, map->size1, map->size2, FALSE);

    // decrease/increase HeapMap
    for (r = 0 ; r < map->size1 ; ++r) {
        for (c = 0 ; c < map->size2 ; ++c) {
            pixel_hit = FALSE;

            // this pixel belongs to certain vehicle candidate.
            for (auto& cand:cands) {
                if (cand.m_valid == FALSE) 
                    continue;        

                if (r >= cand.m_r && r < (cand.m_r + cand.m_h) &&
                    c >= cand.m_c && c < (cand.m_c + cand.m_w)) {
                    pixel_hit = TRUE;
                    break;
                }
            }

            val = gsl_matrix_get(map, r, c);

            if (pixel_hit) {
                val += HeatMapIncrease;
                if (val >= 255)
                    val = 255;
            } else {
                val -= HeatMapDecrease;
                if (val <= 0)
                    val = 0;
            }

            gsl_matrix_set(map, r, c, val);

            if (val < HeatMapAppearThreshold) {
                gsl_matrix_char_set(id, r, c, -1);
            }
        }
    }

    return TRUE;
}

BOOL CFCWS::HeatMapUpdateID(const gsl_matrix* heatmap,
                             gsl_matrix_char* heatmap_id,
                             list<candidate_t>::iterator it,
                             char id)
{
    uint32_t r, c;
    gsl_matrix_view heatmap_sm;
    gsl_matrix_char_view heatmapid_sm;

    if (!heatmap || !heatmap_id) {
        fcwsdbg();
        return FALSE;
    }

    heatmap_sm = gsl_matrix_submatrix((gsl_matrix*)heatmap,
                                      it->m_r,
                                      it->m_c,
                                      it->m_h,
                                      it->m_w);

    heatmapid_sm = gsl_matrix_char_submatrix(heatmap_id,
                                             it->m_r,
                                             it->m_c,
                                             it->m_h,
                                             it->m_w);

    // Update heatmap id within cand region.
    for (r = 0 ; r < heatmap_sm.matrix.size1 ; ++r) {
        for (c = 0 ; c < heatmap_sm.matrix.size2 ; ++c) {
            if (gsl_matrix_get(&heatmap_sm.matrix, r, c) < HeatMapAppearThreshold)
                continue;

            //if (gsl_matrix_char_get(&heatmapid_sm.matrix, rr, cc) != -1)
            //    continue;

            gsl_matrix_char_set(&heatmapid_sm.matrix, r, c, id);
        }
    }

    return TRUE;
}

BOOL CFCWS::HeatMapGetContour(const gsl_matrix_char* m,
                               char  id,
                               const point_t& start,
                               rect& rect)
{
    BOOL ret = FALSE;
    BOOL find_pixel;
    uint32_t r, c;
    uint32_t left, right, top, bottom;
    uint32_t find_fail_cnt;
    uint32_t count;
    DIR nextdir;
    float aspect_ratio;
    point_t tp; // test point to avoid infinite loop.

    if (!m || id < 0) {
        fcwsdbg();
        return ret;
    }

    if (start.r == 0) 
        return ret;

    nextdir = DIR_RIGHTUP;
    find_pixel = FALSE;
    find_fail_cnt = 0;

    r = top = bottom = start.r;
    c = left = right = start.c;

    memset(&tp, 0, sizeof(tp));

    count = 0;
    while (1) {// Get contour of VC. (start point is equal to end point)
        while (1) {// Scan different direction to get neighbour.
            switch (nextdir) {
                case DIR_RIGHTUP:
                    if (gsl_matrix_char_get(m, r-1, c+1) == id) {
                        --r;
                        ++c;
                        find_pixel = TRUE;
                        nextdir = DIR_LEFTUP;

                        top     = top > r ? r : top;
                        right   = right < c ? c : right; 
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_RIGHT;
                    }
                    break;
                case DIR_RIGHT:
                    if (gsl_matrix_char_get(m, r, c+1) == id) {
                        ++c;
                        find_pixel = TRUE;
                        nextdir = DIR_RIGHTUP;

                        right   = right < c ? c : right;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_RIGHTDOWN;
                    }
                    break;
                case DIR_RIGHTDOWN:
                    if (gsl_matrix_char_get(m, r+1, c+1) == id) {
                        ++r;
                        ++c;
                        find_pixel = TRUE;
                        nextdir = DIR_RIGHTUP;

                        bottom  = bottom < r ? r : bottom;
                        right   = right < c ? c : right;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_DOWN;
                    }
                    break;
                case DIR_DOWN:
                    if (gsl_matrix_char_get(m, r+1, c) == id) {
                        ++r;
                        find_pixel = TRUE;
                        nextdir = DIR_RIGHTDOWN;

                        bottom  = bottom < r ? r : bottom;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_LEFTDOWN;
                    }
                    break;
                case DIR_LEFTDOWN:
                    if (gsl_matrix_char_get(m, r+1, c-1) == id) {
                        ++r;
                        --c;
                        find_pixel = TRUE;
                        nextdir = DIR_RIGHTDOWN;

                        bottom  = bottom < r ? r : bottom;
                        left    = left > c ? c : left;
                    } else { 
                        find_fail_cnt++;
                        nextdir = DIR_LEFT;
                    }
                    break;
                case DIR_LEFT:
                    if (gsl_matrix_char_get(m, r, c-1) == id) {
                        --c;
                        find_pixel = TRUE;
                        nextdir = DIR_LEFTDOWN;

                        left    = left > c ? c : left;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_LEFTUP;
                    }
                    break;
                case DIR_LEFTUP:
                    if (gsl_matrix_char_get(m, r-1, c-1) == id) {
                        --r;
                        --c;
                        find_pixel = TRUE;
                        nextdir = DIR_LEFTDOWN;

                        top     = top > r ? r : top;
                        left    = left > c ? c : left;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_UP;
                    }
                    break;
                case DIR_UP:
                    if (gsl_matrix_char_get(m, r-1, c) == id) {
                        --r;
                        find_pixel = TRUE;
                        nextdir = DIR_LEFTUP;

                        top     = top > r ? r : top;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_RIGHTUP;
                    }
                    break;
            } // switch

            if (find_pixel) {
                find_pixel = FALSE;
                find_fail_cnt = 0;
                break;
            }

            if (find_fail_cnt >= DIR_TOTAL) {
                fcwsdbg();
                break;
            }
        }// Scan different direction to get neighbour.

        if (r == start.r && c == start.c) {
           rect.r  = top; 
           rect.c  = left;
           rect.w  = right - left + 1;
           rect.h  = bottom - top + 1;

           // *** Assume previously height is half of weight. *** 
           // aspect ratio checking (VHW_RATIO * 3/4 < ar < VHW_RATIO * 5/4)
           aspect_ratio = (rect.w / (float)rect.h);

           //fcwsdbg("ratio of aspect is %.02f", rect.w / (float)rect.h);
           if (AR_LB < aspect_ratio && aspect_ratio < AR_HB)
               ret = TRUE;
           else
               fcwsdbg("Get inappropriate aspect ratio contour.");

           break;
        } else {
            if (r != start.r && c != start.c) {
                if (tp.r == 0 && tp.c == 0) {
                    tp.r = r;
                    tp.c = c;
                } else {
                    if (r == tp.r && c == tp.c) {
                        //fcwsdbg("start(%d,%d), tp(%d,%d) <.(%d,%d)",
                        //        start.r,
                        //        start.c,
                        //        tp.r,
                        //        tp.c,
                        //        r,
                        //        c);
                        fcwsdbg("Get incorrect contour.");
                        break;
                    }
                }
            }
        }

        if (++count >= (m->size1 * m->size2) >> 4) { // (w/2 * h/2)
            fcwsdbg("Can not generate appropriate contour.");
            break;
        }
    }// Get contour of VC. (start point is equal to end point)

    return ret;
}

BOOL CFCWS::VCTrackerAddOrUpdateExistTarget(const gsl_matrix* heatmap,
                                            gsl_matrix_char* heatmap_id,
                                            list<candidate_t>& tracker,
                                            list<candidate_t>& cands)
{
    char vc_id, max_vc_id;
    uint32_t r, c, rr, cc;
    BOOL gen_new_vc;
    BOOL find_pixel;
    point_t midpoint_newcand;
    point_t contour_sp;
    rect contour_rect;
    gsl_matrix_char_view heatmapid_sm;
    candidate_t newtarget;
    list<candidate_t>::iterator cand, target;

    if (!heatmap || !heatmap_id) {
        fcwsdbg();
        return FALSE;
    }

    if (!cands.size())
        return FALSE;

    //Assign an ID to each pixel which exceeds the threshold and does not have an ID yet.
    for (r = 0 ; r < heatmap->size1 ; ++r) {
        for (c = 0 ; c < heatmap->size2 ; ++c) {
            if (gsl_matrix_get(heatmap, r, c) < HeatMapAppearThreshold)
                continue;

            if (gsl_matrix_char_get(heatmap_id, r, c) != -1)
                continue;

            // which new candidate it belongs to.
            for (cand = cands.begin() ; cand != cands.end() ; ++cand) {
                if (r >= cand->m_r && r < (cand->m_r + cand->m_h) &&
                    c >= cand->m_c && c < (cand->m_c + cand->m_w))
                    break;
            }

            if ((cand == cands.end()) || (cand->m_valid == FALSE))
                continue;

            fcwsdbg("New point at (%d,%d) belongs to new vc[%d]", r, c, cand->m_id);
            gen_new_vc = FALSE;

            // Find the nearest existed candidate(target).
            midpoint_newcand.r = cand->m_r + cand->m_h * .5;
            midpoint_newcand.c = cand->m_c + cand->m_w * .5;

            for (target = tracker.begin() ; target != tracker.end() ; ++target) {
                if (midpoint_newcand.r > target->m_r && midpoint_newcand.r < (target->m_r + target->m_h) &&
                    midpoint_newcand.c > target->m_c && midpoint_newcand.c < (target->m_c + target->m_w)) {
                    break;
                }
            }

            if (target != tracker.end()) {
                // Find a target that is most near to new vc.
                vc_id = target->m_id;    
                fcwsdbg("Find nearest target[%d]", vc_id);

                HeatMapUpdateID(heatmap,
                                heatmap_id,
                                cand,
                                vc_id);

                // set start position of contour.
                if (r > target->m_r && c > target->m_c) {
                    find_pixel = FALSE;
                    for (rr = target->m_r ; rr < target->m_r + target->m_h ; ++rr) {
                        for (cc = target->m_c ; cc < target->m_c + target->m_w ; ++cc) {
                            if (gsl_matrix_char_get(heatmap_id, rr, cc) == target->m_id && gsl_matrix_get(heatmap, rr, cc) >= HeatMapAppearThreshold) {
                                find_pixel = TRUE;
                                break;
                            }
                        }

                        if (find_pixel)
                            break;
                    }

                    contour_sp.r = rr;
                    contour_sp.c = cc;
                    //fcwsdbg("new top_left is inside cur top_left, using (%d,%d) instead of (%d,%d) as start point of contour.", rr, cc, r, c);
                } else {
                    contour_sp.r = r;
                    contour_sp.c = c;
                    //fcwsdbg("new top_left is outside cur top_left, using (%d,%d) as start point of contour.", r, c);
                }
            } else {
                // New candidate needs a new ID.
                vc_id = 0;
                gen_new_vc = TRUE;

                if (tracker.size()) {
                    max_vc_id = 0;

                    for (auto& t:tracker) {
                        if (t.m_id > max_vc_id)
                            max_vc_id = t.m_id;
                    }

                    vc_id = max_vc_id + 1;
                }

                fcwsdbg("Create a new candidate at (%d,%d) with id %d for new vc[%d]", r, c, vc_id, cand->m_id);
                //fcwsdbg("vc %d at (%d,%d) with (%d,%d)", cand->m_id,
                //                                         cand->m_r,
                //                                         cand->m_c,
                //                                         cand->m_w,
                //                                         cand->m_h);

                HeatMapUpdateID(heatmap,
                                heatmap_id,
                                cand,
                                vc_id);

                // set start position of contour.
                contour_sp.r = r;
                contour_sp.c = c;
            }

            if (HeatMapGetContour(heatmap_id, vc_id, contour_sp, contour_rect) == TRUE) {
                if (gen_new_vc) {
                    // Add new target.
                    memset(&newtarget, 0x0, sizeof(candidate_t));

                    newtarget.m_updated     = TRUE;
                    newtarget.m_valid       = TRUE;
                    newtarget.m_id          = vc_id;
                    newtarget.m_r           = contour_rect.r;
                    newtarget.m_c           = contour_rect.c;
                    newtarget.m_w           = contour_rect.w;
                    newtarget.m_h           = contour_rect.h;
                    newtarget.m_dist        = VCGetDist(newtarget.m_w);

                    tracker.push_back(newtarget);
                } else {
                    if (target != tracker.end()) {
                        // Update existed target.
                        target->m_updated     = TRUE;
                        target->m_valid       = TRUE;
                        target->m_id          = vc_id;
                        target->m_r           = contour_rect.r;
                        target->m_c           = contour_rect.c;
                        target->m_w           = contour_rect.w;
                        target->m_h           = contour_rect.h;
                        target->m_dist        = VCGetDist(target->m_w);
                    } else
                        fcwsdbg();
                }

                for (auto& t:tracker) {
                    if (t.m_id == vc_id) {
                        fcwsdbg("target[%d] locates at (%d,%d) with (%d,%d)",
                                t.m_id,
                                t.m_r,
                                t.m_c,
                                t.m_w,
                                t.m_h
                           );

                        heatmapid_sm = gsl_matrix_char_submatrix(heatmap_id,
                                                                 t.m_r,
                                                                 t.m_c,
                                                                 t.m_h,
                                                                 t.m_w);

                        gsl_matrix_char_set_all(&heatmapid_sm.matrix, t.m_id);
                        break;
                    }
                }
            } else {
                if (gen_new_vc) 
                    fcwsdbg("Fail to generate new vc.");
            }
        }
    }

    return TRUE;
}

BOOL CFCWS::VCTrackerUpdateExistTarget(const gsl_matrix_char* heatmap_id,
                                       list<candidate_t>& tracker)
{
    char vc_id;
    uint32_t r, c;
    BOOL find_pixel;
    point_t contour_sp;
    rect contour_rect;

    if (!heatmap_id) {
        fcwsdbg();
        return FALSE;
    }

    if (!tracker.size())
        return FALSE;

    for (list<candidate_t>::iterator t = tracker.begin() ; t != tracker.end() ; ) {
        if (t->m_updated == FALSE) {
            // Find a pixel inside existed target which still has an id.
            find_pixel = FALSE;

            for (r = t->m_r ; r < (t->m_r + t->m_h) ; ++r) {
                for (c = t->m_c ; c < (t->m_c + t->m_w) ; ++c) {
                    if (gsl_matrix_char_get(heatmap_id, r, c) == t->m_id) {
                        find_pixel      = TRUE;
                        contour_sp.r    = r;
                        contour_sp.c    = c;
                        vc_id           = t->m_id;
                        break;
                    }
                }

                if (find_pixel)
                    break;
            }

            if (find_pixel && HeatMapGetContour(heatmap_id, vc_id, contour_sp, contour_rect) == TRUE) {
                t->m_updated     = TRUE;
                t->m_valid       = TRUE;
                t->m_id          = vc_id;
                t->m_r           = contour_rect.r;
                t->m_c           = contour_rect.c;
                t->m_w           = contour_rect.w;
                t->m_h           = contour_rect.h;
                t->m_dist        = VCGetDist(t->m_w);
            }

            if (t->m_updated == TRUE) {
                fcwsdbg("target[%d] locates at (%d,%d) with (%d,%d) is self-updated.",
                        t->m_id,
                        t->m_r,
                        t->m_c,
                        t->m_w,
                        t->m_h);
                ++t;
            } else {
                fcwsdbg("target[%d] is removed.", t->m_id);
                // This existed candidate has not been updated anymore. Remove it from the list.
                t = tracker.erase(t);
            }
        } else
            ++t;
    }

    return TRUE;
}

BOOL CFCWS::VCTrackerUpdate(gsl_matrix* heatmap,
                            gsl_matrix_char* heatmap_id,
                            list<candidate_t>& tracker,
                            list<candidate_t>& cands)
{
    if (!heatmap || !heatmap_id) {
        fcwsdbg();
        return FALSE;
    }

    // Reset each existed candidate as need to update.
    for (auto& target:tracker) {
        fcwsdbg("Tracker: target[%d] at (%d, %d) with (%d, %d)",
                target.m_id,
                target.m_r,
                target.m_c,
                target.m_w,
                target.m_h);

        target.m_updated = FALSE;
    }

    // Dump new vc
    VCDump("New vc", cands);

    VCTrackerAddOrUpdateExistTarget(heatmap, heatmap_id, tracker, cands);

    VCTrackerUpdateExistTarget(heatmap_id, tracker);

    return TRUE;
}

BOOL CFCWS::HypothesisGenerate(const gsl_matrix* imgy,
                               const gsl_matrix* intimg,
                               const gsl_matrix* shadowimg,
                               const gsl_matrix* vedgeimg,
                               const gsl_vector* horizonproject,
                               gsl_vector* temp_horizonproject,
                               gsl_matrix* heatmap,
                               gsl_matrix_char* heatmapid,
                               list<blob_t>& blobs,
                               list<candidate_t>& cands,
                               list<candidate_t>& tracker
                               )
{
    uint32_t cur_peak, cur_peak_idx, max_peak; 

    if (!imgy || !intimg || !shadowimg || !vedgeimg || !horizonproject) {
        fcwsdbg();
        return FALSE;
    }

    CheckOrReallocVector(&temp_horizonproject, horizonproject->size, TRUE);
    gsl_vector_memcpy(temp_horizonproject, horizonproject);

    max_peak = gsl_vector_max(temp_horizonproject);

    if (max_peak == 0)
        return FALSE;

    blobs.clear();
    cands.clear();

    for(;;) {
        cur_peak = gsl_vector_max(temp_horizonproject);
        cur_peak_idx = gsl_vector_max_index(temp_horizonproject);

        if (cur_peak < (max_peak * 0.5))
            break;

        gsl_vector_set(temp_horizonproject, cur_peak_idx, 0);
        BlobGenerate(shadowimg, cur_peak_idx, blobs);
    }

    if (blobs.size()) {
        BlobRearrange(blobs);

        BlobConvertToVC(imgy->size2, blobs, cands);

        VCUpdateShapeByStrongVerticalEdge(vedgeimg, cands);

        VCCheck(imgy, vedgeimg, cands);

        for (list<candidate_t>::iterator cand = cands.begin() ; cand != cands.end() ; ) {
            if (cand->m_valid == TRUE) {

                cand->m_r        = (cand->m_r == 0 ? 1 : cand->m_r);
                cand->m_h        = (cand->m_r == 0 ? cand->m_h - 1 : cand->m_h);
                cand->m_st       = _Disappear;
                ++cand;

            } else 
                cand = cands.erase(cand);
        }
    }

    VCUpdateHeatMap(heatmap, heatmapid, cands);

    VCTrackerUpdate(heatmap, heatmapid, tracker, cands);

    if (tracker.size()) {
        for (auto& t:tracker) {
            fcwsdbg(YELLOW "target %d at (%d,%d) with (%d,%d), dist %.2lf" NONE,
                    t.m_id,
                    t.m_r,
                    t.m_c,
                    t.m_w,
                    t.m_h,
                    t.m_dist);
        }
    }

    return TRUE;
}



















// ------------------public function---------
 
CFCWS::CFCWS()
{
    m_terminate = FALSE;

    m_ip = NULL;

    memset(&m_mutex, 0x0, sizeof(pthread_mutex_t));
    memset(&m_cond, 0x0, sizeof(pthread_cond_t));

    m_imgy      = 
    m_imgu      = 
    m_imgv      = 
    m_vedgeimg  =
    m_heatmap   =
    m_tempimg   =
    m_intimg    =   
    m_shadowimg = NULL;

    m_gradient  = NULL;

    m_direction = 
    m_heatmapid = NULL;

    m_horizonproject        =
    m_temp_horizonproject   = NULL;

    m_blobs.clear();
    m_candidates.clear();
    m_vc_tracker.clear();
}

CFCWS::~CFCWS()
{
    Deinit();
}

BOOL CFCWS::Init()
{
    pthread_mutexattr_t attr;

    m_ip = new CImgProc();

    if (m_ip)
        m_ip->Init();

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_cond_init(&m_cond, NULL);

    return FALSE;
}

BOOL CFCWS::Deinit()
{
    m_terminate = TRUE;

    if (m_ip) {
        delete m_ip;
        m_ip = NULL;
    }

    pthread_mutex_lock(&m_mutex);
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);

    FreeMatrix(&m_imgy);
    FreeMatrix(&m_imgu);
    FreeMatrix(&m_imgv);
    FreeMatrix(&m_vedgeimg);
    FreeMatrix(&m_heatmap);
    FreeMatrix(&m_tempimg);
    FreeMatrix(&m_intimg);
    FreeMatrix(&m_shadowimg);

    FreeMatrix(&m_gradient);

    FreeMatrixChar(&m_direction);
    FreeMatrixChar(&m_heatmapid);

    FreeVector(&m_horizonproject);
    FreeVector(&m_temp_horizonproject);

    m_blobs.clear();
    m_candidates.clear();
    m_vc_tracker.clear();

    return TRUE;
}

BOOL CFCWS::DoDetection(uint8_t* imgy,
                        int w,
                        int h,
                        int linesize,
                        CLDWS* ldws_obj,
                        const roi_t* roi
                        )
{
    if (!m_ip || !imgy || !w || !h || !linesize) {
        fcwsdbg();
        return FALSE;
    }

    pthread_mutex_lock(&m_mutex);

    if (m_terminate == TRUE) {
        pthread_mutex_unlock(&m_mutex);
        return FALSE;
    }

    // Allocate/Clear matrixs.
    CheckOrReallocMatrix(&m_imgy, h, w, TRUE);
    CheckOrReallocMatrix(&m_imgu, h/2, w/2, TRUE);
    CheckOrReallocMatrix(&m_imgv, h/2, w/2, TRUE);
    CheckOrReallocMatrix(&m_vedgeimg, h, w, TRUE);
    CheckOrReallocMatrix(&m_tempimg, h, w, TRUE);
    CheckOrReallocMatrix(&m_intimg, h, w, TRUE);
    CheckOrReallocMatrix(&m_shadowimg, h, w, TRUE);
    CheckOrReallocMatrix(&m_gradient, h, w, TRUE);
    CheckOrReallocMatrixChar(&m_direction, h, w, TRUE);

    // Result will be used for next round.
    CheckOrReallocMatrix(&m_heatmap, h, w, FALSE);
    CheckOrReallocMatrixChar(&m_heatmapid, h, w, FALSE);

    CheckOrReallocVector(&m_horizonproject, h, TRUE);

    m_ip->CopyImage(imgy, m_imgy, w, h, linesize);
    m_ip->CopyImage(imgy, m_tempimg, w, h, linesize);

    // Integral Image & Thresholding
    m_ip->GenIntegralImage(m_imgy, m_intimg);
    m_ip->ThresholdingByIntegralImage(m_imgy, m_intimg, m_shadowimg, 50, 0.7);
 
    // ROI
    if (!ldws_obj || (ldws_obj && ldws_obj->ApplyDynamicROI(m_shadowimg) == FALSE)) {
        ApplyStaticROI(m_shadowimg, roi);
    }
 
    m_ip->CalHorizonProject(m_shadowimg, m_horizonproject);

    m_ip->EdgeDetectForFCW(m_imgy, m_vedgeimg, m_gradient, m_direction, 1);
   
    // Hypothesis Generation
    HypothesisGenerate(m_imgy,
                       m_intimg,
                       m_shadowimg,
                       m_vedgeimg,
                       m_horizonproject,
                       m_temp_horizonproject,
                       m_heatmap,
                       m_heatmapid,
                       m_blobs,
                       m_candidates,
                       m_vc_tracker);


    // Hypothesis Verfication
    // TODO

    pthread_mutex_unlock(&m_mutex);

    return TRUE;
}

BOOL CFCWS::GetInternalData(uint32_t w, 
                            uint32_t h,
                            int linesize,
                            uint8_t* img,
                            uint8_t* shadow,
                            uint8_t* roi,
                            uint8_t* vedge,
                            uint8_t* heatmap,
                            VehicleCandidates* vcs,
                            VehicleCandidates* vcs2
                            )
{
    pthread_mutex_lock(&m_mutex);

    m_ip->CopyBackImage(m_imgy      , img       , w, h, linesize);
    m_ip->CopyBackImage(m_shadowimg , shadow    , w, h, linesize);
    m_ip->CopyBackImage(m_shadowimg , roi       , w, h, linesize);
    m_ip->CopyBackImage(m_vedgeimg  , vedge     , w, h, linesize);
    m_ip->CopyBackImage(m_heatmap   , heatmap   , w, h, linesize);

    // instant vc.
    for (auto& cand:m_candidates) {
        vcs->vc[vcs->vc_count].m_updated    = cand.m_updated;
        vcs->vc[vcs->vc_count].m_valid      = cand.m_valid;
        vcs->vc[vcs->vc_count].m_id         = cand.m_id;
        vcs->vc[vcs->vc_count].m_r          = cand.m_r;
        vcs->vc[vcs->vc_count].m_c          = cand.m_c;
        vcs->vc[vcs->vc_count].m_w          = cand.m_w;
        vcs->vc[vcs->vc_count].m_h          = cand.m_h;
        vcs->vc[vcs->vc_count].m_dist       = cand.m_dist;
        vcs->vc[vcs->vc_count].m_next       = NULL;
        vcs->vc_count++;
    }

    // stable vc
    for (auto& cand:m_vc_tracker) {
        vcs2->vc[vcs2->vc_count].m_updated    = cand.m_updated;
        vcs2->vc[vcs2->vc_count].m_valid      = cand.m_valid;
        vcs2->vc[vcs2->vc_count].m_id         = cand.m_id;
        vcs2->vc[vcs2->vc_count].m_r          = cand.m_r;
        vcs2->vc[vcs2->vc_count].m_c          = cand.m_c;
        vcs2->vc[vcs2->vc_count].m_w          = cand.m_w;
        vcs2->vc[vcs2->vc_count].m_h          = cand.m_h;
        vcs2->vc[vcs2->vc_count].m_dist       = cand.m_dist;
        vcs2->vc[vcs2->vc_count].m_next       = NULL;
        vcs2->vc_count++;
    }

    pthread_mutex_unlock(&m_mutex);

    return TRUE;
}
